#ifndef ASSEMBLY_GEN_H
#define ASSEMBLY_GEN_H

#include <stdio.h>
#include <llvm-c/Core.h>
#include "register_alloc.h"

/* BB labels: LLVMBasicBlockRef -> label string (caller frees labels) */
typedef struct BBLabelsEntry {
	LLVMBasicBlockRef bb;
	char *label;
} BBLabelsEntry;

typedef struct BBLabelsMap {
	BBLabelsEntry *entries;
	int count;
	int capacity;
} BBLabelsMap;

/* offset_map: LLVMValueRef -> offset from %ebp */
typedef struct OffsetMapEntry {
	LLVMValueRef value;
	int offset;
} OffsetMapEntry;

typedef struct OffsetMap {
	OffsetMapEntry *entries;
	int count;
	int capacity;
} OffsetMap;

void bb_labels_init(BBLabelsMap *m);
void bb_labels_clear(BBLabelsMap *m);
void bb_labels_set(BBLabelsMap *m, LLVMBasicBlockRef bb, const char *label);
const char *bb_labels_get(BBLabelsMap *m, LLVMBasicBlockRef bb);

void offset_map_init(OffsetMap *m);
void offset_map_clear(OffsetMap *m);
void offset_map_set(OffsetMap *m, LLVMValueRef value, int offset);
int offset_map_get(OffsetMap *m, LLVMValueRef value);

void createBBLabels(LLVMValueRef function, BBLabelsMap *bb_labels);

void printDirectives(FILE *out, const char *func_name);

void printFunctionEnd(FILE *out);

void getOffsetMap(LLVMModuleRef module, LLVMValueRef function, OffsetMap *offset_map, int *localMem);

void emit_assembly(LLVMModuleRef module, RegMap *reg_map, FILE *out);

#endif
