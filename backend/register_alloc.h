#ifndef REGISTER_ALLOC_H
#define REGISTER_ALLOC_H

#include <llvm-c/Core.h>

/* Only 3 physical registers for allocation: ebx (0), ecx (1), edx (2).
 * eax is reserved: return value, call return, temporary for add/sub/mul/cmp when result is spilled.
 */
#define NUM_PHYSICAL_REGS 3

/* reg_map: LLVMValueRef (value-producing instruction) -> physical reg index (0..2) or -1 (spilled) */
typedef struct RegMapEntry {
	LLVMValueRef value;
	int reg;
} RegMapEntry;

typedef struct RegMap {
	RegMapEntry *entries;
	int count;
	int capacity;
} RegMap;

/* inst_index: instruction -> index in basic block (alloca instructions are not indexed) */
typedef struct InstIndexEntry {
	LLVMValueRef inst;
	int index;
} InstIndexEntry;

typedef struct InstIndexMap {
	InstIndexEntry *entries;
	int count;
	int capacity;
} InstIndexMap;

/* live_range: instruction (that defines a value) -> (start_index, end_index) */
typedef struct LiveRangeEntry {
	LLVMValueRef inst;
	int start;
	int end;
} LiveRangeEntry;

typedef struct LiveRangeMap {
	LiveRangeEntry *entries;
	int count;
	int capacity;
} LiveRangeMap;

void reg_map_init(RegMap *m);
void reg_map_clear(RegMap *m);
void reg_map_set(RegMap *m, LLVMValueRef value, int reg);
int  reg_map_get(RegMap *m, LLVMValueRef value);

void inst_index_map_init(InstIndexMap *m);
void inst_index_map_clear(InstIndexMap *m);
void inst_index_map_set(InstIndexMap *m, LLVMValueRef inst, int index);
int  inst_index_map_get(InstIndexMap *m, LLVMValueRef inst);

void live_range_map_init(LiveRangeMap *m);
void live_range_map_clear(LiveRangeMap *m);
void live_range_map_set(LiveRangeMap *m, LLVMValueRef inst, int start, int end);
int  live_range_map_get_start(LiveRangeMap *m, LLVMValueRef inst);
int  live_range_map_get_end(LiveRangeMap *m, LLVMValueRef inst);


void compute_liveness(LLVMBasicBlockRef bb, InstIndexMap *inst_index, LiveRangeMap *live_range);


LLVMValueRef find_spill(LLVMValueRef Instr, RegMap *reg_map, InstIndexMap *inst_index,
                        LLVMValueRef *sorted_list, int n, LiveRangeMap *live_range);

						
void local_register_allocation(LLVMValueRef function, RegMap *reg_map);

void local_register_allocation_module(LLVMModuleRef module, RegMap *reg_map);

#endif
