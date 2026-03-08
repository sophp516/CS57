#include "register_alloc.h"
#include <stdlib.h>
#include <string.h>

void reg_map_init(RegMap *m) {
	m->entries = NULL;
	m->count = 0;
	m->capacity = 0;
}

void reg_map_clear(RegMap *m) {
	free(m->entries);
	m->entries = NULL;
	m->count = 0;
	m->capacity = 0;
}

// grow the reg_map if the capacity is reached
static void reg_map_grow(RegMap *m) {
	int new_cap = (m->capacity == 0) ? 64 : m->capacity * 2;
	RegMapEntry *p = (RegMapEntry *)realloc(m->entries, (size_t)new_cap * sizeof(RegMapEntry));
	if (!p) return;
	m->entries = p;
	m->capacity = new_cap;
}

// set the value to the reg_map
void reg_map_set(RegMap *m, LLVMValueRef value, int reg) {
	for (int i = 0; i < m->count; i++) {
		if (m->entries[i].value == value) {
			m->entries[i].reg = reg;
			return;
		}
	}
	if (m->count >= m->capacity)
		reg_map_grow(m);
	if (m->count < m->capacity) {
		m->entries[m->count].value = value;
		m->entries[m->count].reg = reg;
		m->count++;
	}
}

int reg_map_get(RegMap *m, LLVMValueRef value) {
	for (int i = 0; i < m->count; i++) {
		if (m->entries[i].value == value)
			return m->entries[i].reg;
	}
	return -2; /* not in map */
}

void inst_index_map_init(InstIndexMap *m) {
	m->entries = NULL;
	m->count = 0;
	m->capacity = 0;
}

void inst_index_map_clear(InstIndexMap *m) {
	free(m->entries);
	m->entries = NULL;
	m->count = 0;
	m->capacity = 0;
}

static void inst_index_map_grow(InstIndexMap *m) {
	int new_cap = (m->capacity == 0) ? 64 : m->capacity * 2;
	InstIndexEntry *p = (InstIndexEntry *)realloc(m->entries, (size_t)new_cap * sizeof(InstIndexEntry));
	if (!p) return;
	m->entries = p;
	m->capacity = new_cap;
}

void inst_index_map_set(InstIndexMap *m, LLVMValueRef inst, int index) {
	for (int i = 0; i < m->count; i++) {
		if (m->entries[i].inst == inst) {
			m->entries[i].index = index;
			return;
		}
	}
	if (m->count >= m->capacity)
		inst_index_map_grow(m);
	if (m->count < m->capacity) {
		m->entries[m->count].inst = inst;
		m->entries[m->count].index = index;
		m->count++;
	}
}

int inst_index_map_get(InstIndexMap *m, LLVMValueRef inst) {
	for (int i = 0; i < m->count; i++) {
		if (m->entries[i].inst == inst)
			return m->entries[i].index;
	}
	return -1;
}

void live_range_map_init(LiveRangeMap *m) {
	m->entries = NULL;
	m->count = 0;
	m->capacity = 0;
}

void live_range_map_clear(LiveRangeMap *m) {
	free(m->entries);
	m->entries = NULL;
	m->count = 0;
	m->capacity = 0;
}

static void live_range_map_grow(LiveRangeMap *m) {
	int new_cap = (m->capacity == 0) ? 64 : m->capacity * 2;
	LiveRangeEntry *p = (LiveRangeEntry *)realloc(m->entries, (size_t)new_cap * sizeof(LiveRangeEntry));
	if (!p) return;
	m->entries = p;
	m->capacity = new_cap;
}

void live_range_map_set(LiveRangeMap *m, LLVMValueRef inst, int start, int end) {
	for (int i = 0; i < m->count; i++) {
		if (m->entries[i].inst == inst) {
			m->entries[i].start = start;
			m->entries[i].end = end;
			return;
		}
	}
	if (m->count >= m->capacity)
		live_range_map_grow(m);
	if (m->count < m->capacity) {
		m->entries[m->count].inst = inst;
		m->entries[m->count].start = start;
		m->entries[m->count].end = end;
		m->count++;
	}
}

int live_range_map_get_start(LiveRangeMap *m, LLVMValueRef inst) {
	for (int i = 0; i < m->count; i++) {
		if (m->entries[i].inst == inst)
			return m->entries[i].start;
	}
	return -1;
}

int live_range_map_get_end(LiveRangeMap *m, LLVMValueRef inst) {
	for (int i = 0; i < m->count; i++) {
		if (m->entries[i].inst == inst)
			return m->entries[i].end;
	}
	return -1;
}

// helpers
static int is_alloca(LLVMValueRef inst) {
	return LLVMGetInstructionOpcode(inst) == LLVMAlloca;
}

static int instruction_produces_value(LLVMValueRef inst) {
	if (LLVMIsATerminatorInst(inst))
		return 0;
	LLVMOpcode op = LLVMGetInstructionOpcode(inst);
	if (op == LLVMStore)
		return 0;
	if (op == LLVMCall || op == LLVMInvoke) {
		LLVMTypeRef ty = LLVMTypeOf(inst);
		return LLVMGetTypeKind(ty) != LLVMVoidTypeKind;
	}
	return 1;
}

static int is_add_sub_mul(LLVMValueRef inst) {
	LLVMOpcode op = LLVMGetInstructionOpcode(inst);
	return op == LLVMAdd || op == LLVMSub || op == LLVMMul;
}

// count uses of value (for heuristic)
static int count_uses(LLVMValueRef value) {
	int n = 0;
	for (LLVMUseRef u = LLVMGetFirstUse(value); u; u = LLVMGetNextUse(u))
		n++;
	return n;
}

// compute liveness
void compute_liveness(LLVMBasicBlockRef bb, InstIndexMap *inst_index, LiveRangeMap *live_range) {
	inst_index_map_clear(inst_index);
	live_range_map_clear(live_range);

	int idx = 0;
	for (LLVMValueRef inst = LLVMGetFirstInstruction(bb); inst;
	     inst = LLVMGetNextInstruction(inst)) {
		if (is_alloca(inst))
			continue;
		inst_index_map_set(inst_index, inst, idx);
		idx++;
	}

	for (LLVMValueRef inst = LLVMGetFirstInstruction(bb); inst;
	     inst = LLVMGetNextInstruction(inst)) {
		if (is_alloca(inst))
			continue;
		if (!instruction_produces_value(inst))
			continue;
		int def_idx = inst_index_map_get(inst_index, inst);
		if (def_idx < 0)
			continue;

		LLVMValueRef last_use_inst = NULL;
		for (LLVMValueRef i = LLVMGetFirstInstruction(bb); i;
		     i = LLVMGetNextInstruction(i)) {
			if (is_alloca(i))
				continue;
			int n = LLVMGetNumOperands(i);
			for (int j = 0; j < n; j++) {
				if (LLVMGetOperand(i, j) == inst) {
					last_use_inst = i;
					break;
				}
			}
		}
		int end_idx = (last_use_inst != NULL) ? inst_index_map_get(inst_index, last_use_inst) : def_idx;
		live_range_map_set(live_range, inst, def_idx, end_idx);
	}
}

// overlap: [s1,e1] and [s2,e2] overlap iff s1 <= e2 && s2 <= e1
static int ranges_overlap(int s1, int e1, int s2, int e2) {
	return s1 <= e2 && s2 <= e1;
}

LLVMValueRef find_spill(LLVMValueRef Instr, RegMap *reg_map, InstIndexMap *inst_index,
                        LLVMValueRef *sorted_list, int n, LiveRangeMap *live_range) {
	int si = live_range_map_get_start(live_range, Instr);
	int ei = live_range_map_get_end(live_range, Instr);
	for (int i = 0; i < n; i++) {
		LLVMValueRef V = sorted_list[i];
		int r = reg_map_get(reg_map, V);
		if (r < 0)
			continue;
		int sv = live_range_map_get_start(live_range, V);
		int ev = live_range_map_get_end(live_range, V);
		if (sv < 0 || ev < 0)
			continue;
		if (!ranges_overlap(si, ei, sv, ev))
			continue;
		return V;
	}
	return NULL;
}

// add register to available set (bitmask or array)
// ie use array of 3: available[0]=ebx, available[1]=ecx, available[2]=edx
static void add_to_available(int *available, int reg) {
	if (reg >= 0 && reg < NUM_PHYSICAL_REGS)
		available[reg] = 1;
}

static int take_available(int *available) {
	for (int i = 0; i < NUM_PHYSICAL_REGS; i++) {
		if (available[i]) {
			available[i] = 0;
			return i;
		}
	}
	return -1;
}

// free operands of Instr whose live range ends at current index (only for operands in this block)
static void free_operands_ending_at(LLVMValueRef Instr, int current_index,
                                    RegMap *reg_map, InstIndexMap *inst_index, LiveRangeMap *live_range,
                                    int *available) {
	int num = LLVMGetNumOperands(Instr);
	for (int i = 0; i < num; i++) {
		LLVMValueRef op = LLVMGetOperand(Instr, i);
		if (inst_index_map_get(inst_index, op) < 0)
			continue;
		int end = live_range_map_get_end(live_range, op);
		if (end == current_index) {
			int r = reg_map_get(reg_map, op);
			if (r >= 0)
				add_to_available(available, r);
		}
	}
}


// my chosen heuristic: sort by live end decreasing (spill the one that ends latest first).
static int build_sorted_list_for_spill(LLVMBasicBlockRef bb, LLVMValueRef Instr,
                                       LiveRangeMap *live_range, LLVMValueRef *out_list, int max_n) {
	LLVMValueRef *buf = (LLVMValueRef *)malloc((size_t)max_n * sizeof(LLVMValueRef));
	if (!buf) return 0;
	int n = 0;
	int si = live_range_map_get_start(live_range, Instr);
	int ei = live_range_map_get_end(live_range, Instr);

	for (LLVMValueRef inst = LLVMGetFirstInstruction(bb); inst && n < max_n;
	     inst = LLVMGetNextInstruction(inst)) {
		if (is_alloca(inst) || inst == Instr)
			continue;
		if (!instruction_produces_value(inst))
			continue;
		int s = live_range_map_get_start(live_range, inst);
		int e = live_range_map_get_end(live_range, inst);
		if (s < 0 || e < 0) continue;
		if (!ranges_overlap(si, ei, s, e))
			continue;
		buf[n++] = inst;
	}

	for (int i = 0; i < n - 1; i++) {
		for (int j = i + 1; j < n; j++) {
			int ei = live_range_map_get_end(live_range, buf[i]);
			int ej = live_range_map_get_end(live_range, buf[j]);
			if (ej > ei) {
				LLVMValueRef t = buf[i];
				buf[i] = buf[j];
				buf[j] = t;
			}
		}
	}
	for (int i = 0; i < n; i++)
		out_list[i] = buf[i];
	free(buf);
	return n;
}

static void allocate_block(LLVMBasicBlockRef bb, RegMap *reg_map) {
	InstIndexMap inst_index;
	LiveRangeMap live_range;
	inst_index_map_init(&inst_index);
	live_range_map_init(&live_range);

	compute_liveness(bb, &inst_index, &live_range);

	int available[NUM_PHYSICAL_REGS];
	for (int i = 0; i < NUM_PHYSICAL_REGS; i++)
		available[i] = 1;

	LLVMValueRef *sorted_for_spill = (LLVMValueRef *)malloc(256 * sizeof(LLVMValueRef));
	if (!sorted_for_spill) {
		inst_index_map_clear(&inst_index);
		live_range_map_clear(&live_range);
		return;
	}

	for (LLVMValueRef Instr = LLVMGetFirstInstruction(bb); Instr;
	     Instr = LLVMGetNextInstruction(Instr)) {

		if (is_alloca(Instr))
			continue;

		int cur_idx = inst_index_map_get(&inst_index, Instr);

		if (!instruction_produces_value(Instr)) {
			free_operands_ending_at(Instr, cur_idx, reg_map, &inst_index, &live_range, available);
			continue;
		}

		// value-producing instruction
		LLVMValueRef op0 = (LLVMGetNumOperands(Instr) >= 1) ? LLVMGetOperand(Instr, 0) : NULL;
		LLVMValueRef op1 = (LLVMGetNumOperands(Instr) >= 2) ? LLVMGetOperand(Instr, 1) : NULL;

		int end0 = (op0 && inst_index_map_get(&inst_index, op0) >= 0) ? live_range_map_get_end(&live_range, op0) : -1;
		int r0 = (op0 && inst_index_map_get(&inst_index, op0) >= 0) ? reg_map_get(reg_map, op0) : -2;

		int assigned = -1;

		if (is_add_sub_mul(Instr) && op0 && r0 >= 0 && end0 == cur_idx) {
			reg_map_set(reg_map, Instr, r0);
			assigned = r0;
			if (op1 && inst_index_map_get(&inst_index, op1) >= 0) {
				int end1 = live_range_map_get_end(&live_range, op1);
				if (end1 == cur_idx) {
					int r1 = reg_map_get(reg_map, op1);
					if (r1 >= 0)
						add_to_available(available, r1);
				}
			}
		} else {
			int r = take_available(available);
			if (r >= 0) {
				reg_map_set(reg_map, Instr, r);
				assigned = r;
				free_operands_ending_at(Instr, cur_idx, reg_map, &inst_index, &live_range, available);
			} else {
				int n = build_sorted_list_for_spill(bb, Instr, &live_range, sorted_for_spill, 256);
				LLVMValueRef V = find_spill(Instr, reg_map, &inst_index, sorted_for_spill, n, &live_range);
				int end_instr = live_range_map_get_end(&live_range, Instr);
				int end_v = V ? live_range_map_get_end(&live_range, V) : -1;
				int uses_instr = count_uses(Instr);
				int uses_v = V ? count_uses(V) : 0;

				if (V == NULL || (end_instr > end_v) || (uses_instr < uses_v)) {
					reg_map_set(reg_map, Instr, -1);
				} else {
					int R = reg_map_get(reg_map, V);
					reg_map_set(reg_map, Instr, R);
					reg_map_set(reg_map, V, -1);
				}
				free_operands_ending_at(Instr, cur_idx, reg_map, &inst_index, &live_range, available);
			}
		}
	}

	free(sorted_for_spill);
	inst_index_map_clear(&inst_index);
	live_range_map_clear(&live_range);
}

void local_register_allocation(LLVMValueRef function, RegMap *reg_map) {
	reg_map_clear(reg_map);
	for (LLVMBasicBlockRef bb = LLVMGetFirstBasicBlock(function); bb;
	     bb = LLVMGetNextBasicBlock(bb)) {
		allocate_block(bb, reg_map);
	}
}

void local_register_allocation_module(LLVMModuleRef module, RegMap *reg_map) {
	reg_map_clear(reg_map);
	for (LLVMValueRef function = LLVMGetFirstFunction(module); function;
	     function = LLVMGetNextFunction(function)) {
		for (LLVMBasicBlockRef bb = LLVMGetFirstBasicBlock(function); bb;
		     bb = LLVMGetNextBasicBlock(bb)) {
			allocate_block(bb, reg_map);
		}
	}
}
