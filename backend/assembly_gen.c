/*
 * x86 32-bit assembly code generation from LLVM IR.
 * Uses reg_map (from local register allocation) and offset_map.
 */

#include "assembly_gen.h"
#include "register_alloc.h"
#include <stdlib.h>
#include <string.h>
#include <llvm-c/Core.h>

static const char *reg_name(int r) {
	if (r >= 0 && r < NUM_PHYSICAL_REGS) {
		static const char *names[] = { "%ebx", "%ecx", "%edx" };
		return names[r];
	}
	return "%eax";
}

/* --- BBLabelsMap --- */
static void bb_labels_grow(BBLabelsMap *m) {
	int new_cap = (m->capacity == 0) ? 32 : m->capacity * 2;
	BBLabelsEntry *p = (BBLabelsEntry *)realloc(m->entries, (size_t)new_cap * sizeof(BBLabelsEntry));
	if (!p) return;
	m->entries = p;
	m->capacity = new_cap;
}

void bb_labels_init(BBLabelsMap *m) {
	m->entries = NULL;
	m->count = 0;
	m->capacity = 0;
}

void bb_labels_clear(BBLabelsMap *m) {
	for (int i = 0; i < m->count; i++)
		free(m->entries[i].label);
	free(m->entries);
	m->entries = NULL;
	m->count = 0;
	m->capacity = 0;
}

void bb_labels_set(BBLabelsMap *m, LLVMBasicBlockRef bb, const char *label) {
	for (int i = 0; i < m->count; i++) {
		if (m->entries[i].bb == bb) {
			free(m->entries[i].label);
			size_t len = strlen(label) + 1;
			m->entries[i].label = (char *)malloc(len);
			if (m->entries[i].label)
				memcpy(m->entries[i].label, label, len);
			return;
		}
	}
	if (m->count >= m->capacity)
		bb_labels_grow(m);
	if (m->count < m->capacity) {
		m->entries[m->count].bb = bb;
		size_t len = strlen(label) + 1;
		m->entries[m->count].label = (char *)malloc(len);
		if (m->entries[m->count].label)
			memcpy(m->entries[m->count].label, label, len);
		m->count++;
	}
}

const char *bb_labels_get(BBLabelsMap *m, LLVMBasicBlockRef bb) {
	for (int i = 0; i < m->count; i++)
		if (m->entries[i].bb == bb)
			return m->entries[i].label;
	return NULL;
}

/* --- OffsetMap --- */
static void offset_map_grow(OffsetMap *m) {
	int new_cap = (m->capacity == 0) ? 64 : m->capacity * 2;
	OffsetMapEntry *p = (OffsetMapEntry *)realloc(m->entries, (size_t)new_cap * sizeof(OffsetMapEntry));
	if (!p) return;
	m->entries = p;
	m->capacity = new_cap;
}

void offset_map_init(OffsetMap *m) {
	m->entries = NULL;
	m->count = 0;
	m->capacity = 0;
}

void offset_map_clear(OffsetMap *m) {
	free(m->entries);
	m->entries = NULL;
	m->count = 0;
	m->capacity = 0;
}

void offset_map_set(OffsetMap *m, LLVMValueRef value, int offset) {
	for (int i = 0; i < m->count; i++) {
		if (m->entries[i].value == value) {
			m->entries[i].offset = offset;
			return;
		}
	}
	if (m->count >= m->capacity)
		offset_map_grow(m);
	if (m->count < m->capacity) {
		m->entries[m->count].value = value;
		m->entries[m->count].offset = offset;
		m->count++;
	}
}

int offset_map_get(OffsetMap *m, LLVMValueRef value) {
	for (int i = 0; i < m->count; i++)
		if (m->entries[i].value == value)
			return m->entries[i].offset;
	return 0;
}

/* --- createBBLabels --- */
void createBBLabels(LLVMValueRef function, BBLabelsMap *bb_labels) {
	bb_labels_clear(bb_labels);
	const char *func_name = LLVMGetValueName(function);
	if (!func_name) func_name = "func";
	int idx = 0;
	for (LLVMBasicBlockRef bb = LLVMGetFirstBasicBlock(function); bb;
	     bb = LLVMGetNextBasicBlock(bb)) {
		char label[128];
		snprintf(label, sizeof(label), ".L%s_%d", func_name, idx++);
		bb_labels_set(bb_labels, bb, label);
	}
}

/* --- printDirectives --- */
void printDirectives(FILE *out, const char *func_name) {
	fprintf(out, "\t.text\n");
	fprintf(out, "\t.section\t.note.GNU-stack,\"\",@progbits\n");
	fprintf(out, "\t.globl\t%s\n", func_name);
	fprintf(out, "\t.type\t%s, @function\n", func_name);
	fprintf(out, "%s:\n", func_name);
}

/* --- printFunctionEnd --- */
void printFunctionEnd(FILE *out) {
	fprintf(out, "\tleave\n");
	fprintf(out, "\tret\n");
}

/* --- getOffsetMap --- */
void getOffsetMap(LLVMModuleRef module, LLVMValueRef function, OffsetMap *offset_map, int *localMem) {
	(void)module;
	offset_map_clear(offset_map);
	*localMem = 4;

	LLVMValueRef param = LLVMGetParam(function, 0);
	if (param) {
		offset_map_set(offset_map, param, 8);
	}

	for (LLVMBasicBlockRef bb = LLVMGetFirstBasicBlock(function); bb;
	     bb = LLVMGetNextBasicBlock(bb)) {
		for (LLVMValueRef inst = LLVMGetFirstInstruction(bb); inst;
		     inst = LLVMGetNextInstruction(inst)) {
			LLVMOpcode op = LLVMGetInstructionOpcode(inst);

			if (op == LLVMAlloca) {
				*localMem += 4;
				offset_map_set(offset_map, inst, -(*localMem));
			} else if (op == LLVMStore) {
				LLVMValueRef val = LLVMGetOperand(inst, 0);
				LLVMValueRef ptr = LLVMGetOperand(inst, 1);
				if (param && val == param) {
					int x = offset_map_get(offset_map, val);
					offset_map_set(offset_map, ptr, x);
				} else {
					int x = offset_map_get(offset_map, ptr);
					offset_map_set(offset_map, val, x);
				}
			} else if (op == LLVMLoad) {
				LLVMValueRef ptr = LLVMGetOperand(inst, 0);
				int x = offset_map_get(offset_map, ptr);
				offset_map_set(offset_map, inst, x);
			}
		}
	}
}

/* Helper: is value the function parameter? */
static int is_param(LLVMValueRef function, LLVMValueRef value) {
	LLVMValueRef p = LLVMGetParam(function, 0);
	return (p == value);
}

/* Helper: get constant int or -999999 if not constant */
static long long get_const_int(LLVMValueRef value) {
	if (!value) return -999999;
	if (!LLVMIsConstant(value)) return -999999;
	if (LLVMGetValueKind(value) != LLVMConstantIntValueKind) return -999999;
	return (long long)LLVMConstIntGetSExtValue(value);
}

static void emit_ret(LLVMValueRef function, LLVMValueRef inst, RegMap *reg_map, OffsetMap *offset_map, FILE *out) {
	LLVMValueRef A = LLVMGetOperand(inst, 0);
	long long c = get_const_int(A);
	if (c != -999999) {
		fprintf(out, "\tmovl\t$%lld, %%eax\n", c);
	} else {
		int k = offset_map_get(offset_map, A);
		int r = reg_map_get(reg_map, A);
		if (r >= 0) {
			fprintf(out, "\tmovl\t%s, %%eax\n", reg_name(r));
		} else {
			fprintf(out, "\tmovl\t%d(%%ebp), %%eax\n", k);
		}
	}
	fprintf(out, "\tpopl\t%%ebx\n");
	printFunctionEnd(out);
}

static void emit_load(LLVMValueRef inst, RegMap *reg_map, OffsetMap *offset_map, FILE *out) {
	int r = reg_map_get(reg_map, inst);
	if (r < 0) return;
	LLVMValueRef ptr = LLVMGetOperand(inst, 0);
	int c = offset_map_get(offset_map, ptr);
	fprintf(out, "\tmovl\t%d(%%ebp), %s\n", c, reg_name(r));
}

static void emit_store(LLVMValueRef function, LLVMValueRef inst, RegMap *reg_map, OffsetMap *offset_map, FILE *out) {
	LLVMValueRef A = LLVMGetOperand(inst, 0);
	LLVMValueRef ptr = LLVMGetOperand(inst, 1);
	if (is_param(function, A))
		return;
	long long cval = get_const_int(A);
	if (cval != -999999) {
		int c = offset_map_get(offset_map, ptr);
		fprintf(out, "\tmovl\t$%lld, %d(%%ebp)\n", cval, c);
		return;
	}
	int r = reg_map_get(reg_map, A);
	int c = offset_map_get(offset_map, ptr);
	if (r >= 0) {
		fprintf(out, "\tmovl\t%s, %d(%%ebp)\n", reg_name(r), c);
	} else {
		int c1 = offset_map_get(offset_map, A);
		fprintf(out, "\tmovl\t%d(%%ebp), %%eax\n", c1);
		fprintf(out, "\tmovl\t%%eax, %d(%%ebp)\n", c);
	}
}

static void emit_call(LLVMValueRef inst, RegMap *reg_map, OffsetMap *offset_map, FILE *out) {
	LLVMValueRef callee = LLVMGetCalledValue(inst);
	const char *func_name = callee ? LLVMGetValueName(callee) : NULL;
	if (!func_name) func_name = "func";
	int num_args = LLVMGetNumOperands(inst);

	fprintf(out, "\tpushl\t%%ecx\n");
	fprintf(out, "\tpushl\t%%edx\n");

	if (num_args >= 1) {
		LLVMValueRef P = LLVMGetOperand(inst, 0);
		long long cval = get_const_int(P);
		if (cval != -999999) {
			fprintf(out, "\tpushl\t$%lld\n", cval);
		} else {
			int r = reg_map_get(reg_map, P);
			if (r >= 0) {
				fprintf(out, "\tpushl\t%s\n", reg_name(r));
			} else {
				int k = offset_map_get(offset_map, P);
				fprintf(out, "\tpushl\t%d(%%ebp)\n", k);
			}
		}
	}

	fprintf(out, "\tcall\t%s\n", func_name);

	if (num_args >= 1)
		fprintf(out, "\taddl\t$4, %%esp\n");
	fprintf(out, "\tpopl\t%%edx\n");
	fprintf(out, "\tpopl\t%%ecx\n");

	/* Call returns a value? */
	if (LLVMGetTypeKind(LLVMTypeOf(inst)) != LLVMVoidTypeKind) {
		int r = reg_map_get(reg_map, inst);
		if (r >= 0) {
			fprintf(out, "\tmovl\t%%eax, %s\n", reg_name(r));
		} else {
			int k = offset_map_get(offset_map, inst);
			fprintf(out, "\tmovl\t%%eax, %d(%%ebp)\n", k);
		}
	}
}

/* Branch: unconditional jmp or conditional (test i1, jne then_label; jmp else_label). */
static void emit_branch(LLVMValueRef inst, RegMap *reg_map, OffsetMap *offset_map,
                        BBLabelsMap *bb_labels, FILE *out) {
	int num_ops = LLVMGetNumOperands(inst);
	if (num_ops == 1) {
		LLVMBasicBlockRef target = (LLVMBasicBlockRef)LLVMGetOperand(inst, 0);
		const char *L = bb_labels_get(bb_labels, target);
		if (L) fprintf(out, "\tjmp\t%s\n", L);
		return;
	}
	/* Conditional: op0 = condition, op1 = else block, op2 = then block */
	LLVMBasicBlockRef else_blk = (LLVMBasicBlockRef)LLVMGetOperand(inst, 1);
	LLVMBasicBlockRef then_blk = (LLVMBasicBlockRef)LLVMGetOperand(inst, 2);
	const char *L1 = bb_labels_get(bb_labels, then_blk);
	const char *L2 = bb_labels_get(bb_labels, else_blk);
	if (!L1 || !L2) return;

	LLVMValueRef cond_val = LLVMGetOperand(inst, 0);
	int r = reg_map_get(reg_map, cond_val);
	if (r >= 0)
		fprintf(out, "\tmovl\t%s, %%eax\n", reg_name(r));
	else if (offset_map) {
		int k = offset_map_get(offset_map, cond_val);
		if (k != 0)
			fprintf(out, "\tmovl\t%d(%%ebp), %%eax\n", k);
	}
	fprintf(out, "\tcmpl\t$0, %%eax\n");
	fprintf(out, "\tjne\t%s\n", L1);
	fprintf(out, "\tjmp\t%s\n", L2);
}

static void emit_arith(LLVMValueRef inst, RegMap *reg_map, OffsetMap *offset_map, FILE *out) {
	LLVMOpcode op = LLVMGetInstructionOpcode(inst);
	const char *op_name = (op == LLVMAdd) ? "addl" : (op == LLVMSub) ? "subl" : "imull";

	LLVMValueRef A = LLVMGetOperand(inst, 0);
	LLVMValueRef B = LLVMGetOperand(inst, 1);
	int r_result = reg_map_get(reg_map, inst);
	const char *R = (r_result >= 0) ? reg_name(r_result) : "%eax";

	long long cA = get_const_int(A);
	if (cA != -999999) {
		fprintf(out, "\tmovl\t$%lld, %s\n", cA, R);
	} else {
		int rA = reg_map_get(reg_map, A);
		if (rA >= 0) {
			if (rA != r_result || strcmp(reg_name(rA), R) != 0)
				fprintf(out, "\tmovl\t%s, %s\n", reg_name(rA), R);
		} else {
			int n = offset_map_get(offset_map, A);
			fprintf(out, "\tmovl\t%d(%%ebp), %s\n", n, R);
		}
	}

	long long cB = get_const_int(B);
	if (cB != -999999) {
		fprintf(out, "\t%s\t$%lld, %s\n", op_name, cB, R);
	} else {
		int rB = reg_map_get(reg_map, B);
		if (rB >= 0) {
			fprintf(out, "\t%s\t%s, %s\n", op_name, reg_name(rB), R);
		} else {
			int m = offset_map_get(offset_map, B);
			fprintf(out, "\t%s\t%d(%%ebp), %s\n", op_name, m, R);
		}
	}

	if (r_result < 0) {
		int k = offset_map_get(offset_map, inst);
		fprintf(out, "\tmovl\t%%eax, %d(%%ebp)\n", k);
	}
}

static void emit_icmp(LLVMValueRef inst, RegMap *reg_map, OffsetMap *offset_map, FILE *out) {
	LLVMValueRef A = LLVMGetOperand(inst, 0);
	LLVMValueRef B = LLVMGetOperand(inst, 1);
	int r_result = reg_map_get(reg_map, inst);
	const char *R = (r_result >= 0) ? reg_name(r_result) : "%eax";

	long long cA = get_const_int(A);
	if (cA != -999999) {
		fprintf(out, "\tmovl\t$%lld, %s\n", cA, R);
	} else {
		int rA = reg_map_get(reg_map, A);
		if (rA >= 0) {
			if (rA != r_result || strcmp(reg_name(rA), R) != 0)
				fprintf(out, "\tmovl\t%s, %s\n", reg_name(rA), R);
		} else {
			int n = offset_map_get(offset_map, A);
			fprintf(out, "\tmovl\t%d(%%ebp), %s\n", n, R);
		}
	}

	long long cB = get_const_int(B);
	if (cB != -999999) {
		fprintf(out, "\tcmpl\t$%lld, %s\n", cB, R);
	} else {
		int rB = reg_map_get(reg_map, B);
		if (rB >= 0) {
			fprintf(out, "\tcmpl\t%s, %s\n", reg_name(rB), R);
		} else {
			int m = offset_map_get(offset_map, B);
			fprintf(out, "\tcmpl\t%d(%%ebp), %s\n", m, R);
		}
	}

	LLVMIntPredicate pred = LLVMGetICmpPredicate(inst);
	const char *setcc = "sete";
	switch (pred) {
		case 32: setcc = "sete"; break;
		case 33: setcc = "setne"; break;
		case 38: setcc = "setg"; break;
		case 39: setcc = "setge"; break;
		case 40: setcc = "setl"; break;
		case 41: setcc = "setle"; break;
		default: setcc = "sete"; break;
	}
	fprintf(out, "\t%s\t%%al\n", setcc);
	fprintf(out, "\tmovzbl\t%%al, %s\n", R);
}

void emit_assembly(LLVMModuleRef module, RegMap *reg_map, FILE *out) {
	for (LLVMValueRef function = LLVMGetFirstFunction(module); function;
	     function = LLVMGetNextFunction(function)) {

		if (!LLVMGetFirstBasicBlock(function))
			continue;

		const char *func_name = LLVMGetValueName(function);
		if (!func_name) func_name = "func";

		BBLabelsMap bb_labels;
		bb_labels_init(&bb_labels);
		createBBLabels(function, &bb_labels);

		printDirectives(out, func_name);

		OffsetMap offset_map;
		int localMem;
		offset_map_init(&offset_map);
		getOffsetMap(module, function, &offset_map, &localMem);

		fprintf(out, "\tpushl\t%%ebp\n");
		fprintf(out, "\tmovl\t%%esp, %%ebp\n");
		fprintf(out, "\tsubl\t$%d, %%esp\n", localMem);
		fprintf(out, "\tpushl\t%%ebx\n");

		LLVMBasicBlockRef entry_bb = LLVMGetEntryBasicBlock(function);

		/* Emit entry block first so execution starts there */
		{
			LLVMBasicBlockRef bb = entry_bb;
			const char *label = bb_labels_get(&bb_labels, bb);
			if (label)
				fprintf(out, "%s:\n", label);
			for (LLVMValueRef Instr = LLVMGetFirstInstruction(bb); Instr;
			     Instr = LLVMGetNextInstruction(Instr)) {
				LLVMOpcode op = LLVMGetInstructionOpcode(Instr);
				if (op == LLVMRet) {
					emit_ret(function, Instr, reg_map, &offset_map, out);
					bb_labels_clear(&bb_labels);
					offset_map_clear(&offset_map);
					return;
				}
				if (op == LLVMLoad) { emit_load(Instr, reg_map, &offset_map, out); continue; }
				if (op == LLVMStore) { emit_store(function, Instr, reg_map, &offset_map, out); continue; }
				if (op == LLVMCall || op == LLVMInvoke) { emit_call(Instr, reg_map, &offset_map, out); continue; }
				if (op == LLVMBr) { emit_branch(Instr, reg_map, &offset_map, &bb_labels, out); continue; }
				if (op == LLVMAdd || op == LLVMSub || op == LLVMMul) { emit_arith(Instr, reg_map, &offset_map, out); continue; }
				if (op == LLVMICmp) { emit_icmp(Instr, reg_map, &offset_map, out); continue; }
			}
		}

		for (LLVMBasicBlockRef bb = LLVMGetFirstBasicBlock(function); bb;
		     bb = LLVMGetNextBasicBlock(bb)) {
			if (bb == entry_bb)
				continue;
			const char *label = bb_labels_get(&bb_labels, bb);
			if (label)
				fprintf(out, "%s:\n", label);
			for (LLVMValueRef Instr = LLVMGetFirstInstruction(bb); Instr;
			     Instr = LLVMGetNextInstruction(Instr)) {
				LLVMOpcode op = LLVMGetInstructionOpcode(Instr);
				if (op == LLVMRet) {
					emit_ret(function, Instr, reg_map, &offset_map, out);
					bb_labels_clear(&bb_labels);
					offset_map_clear(&offset_map);
					return;
				}
				if (op == LLVMLoad) { emit_load(Instr, reg_map, &offset_map, out); continue; }
				if (op == LLVMStore) { emit_store(function, Instr, reg_map, &offset_map, out); continue; }
				if (op == LLVMCall || op == LLVMInvoke) { emit_call(Instr, reg_map, &offset_map, out); continue; }
				if (op == LLVMBr) { emit_branch(Instr, reg_map, &offset_map, &bb_labels, out); continue; }
				if (op == LLVMAdd || op == LLVMSub || op == LLVMMul) { emit_arith(Instr, reg_map, &offset_map, out); continue; }
				if (op == LLVMICmp) { emit_icmp(Instr, reg_map, &offset_map, out); continue; }
			}
		}

		bb_labels_clear(&bb_labels);
		offset_map_clear(&offset_map);
	}
}
