/*
 * Backend driver: read LLVM IR (.ll), run register allocation, emit x86-32 assembly (.s).
 * Usage: backend_driver input.ll output.s
 */

#include <stdio.h>
#include <stdlib.h>
#include <llvm-c/Core.h>
#include <llvm-c/IRReader.h>
#include "register_alloc.h"
#include "assembly_gen.h"

int main(int argc, char **argv) {
	if (argc != 3) {
		fprintf(stderr, "Usage: %s <input.ll> <output.s>\n", argv[0]);
		return 1;
	}

	char *err = NULL;
	LLVMMemoryBufferRef buf = NULL;
	LLVMModuleRef module = NULL;

	if (LLVMCreateMemoryBufferWithContentsOfFile(argv[1], &buf, &err) != 0 || !buf) {
		if (err) fprintf(stderr, "%s\n", err);
		else fprintf(stderr, "Failed to read file: %s\n", argv[1]);
		return 1;
	}

	if (LLVMParseIRInContext(LLVMGetGlobalContext(), buf, &module, &err) != 0 || !module) {
		if (err) fprintf(stderr, "%s\n", err);
		else fprintf(stderr, "Failed to parse IR\n");
		LLVMDisposeMemoryBuffer(buf);
		return 1;
	}
	/* Do not call LLVMDisposeMemoryBuffer(buf) - can cause segfault when module holds references */

	FILE *out = fopen(argv[2], "w");
	if (!out) {
		fprintf(stderr, "Failed to open output: %s\n", argv[2]);
		LLVMDisposeModule(module);
		return 1;
	}
	RegMap reg_map;
	reg_map_init(&reg_map);
	local_register_allocation_module(module, &reg_map);
	emit_assembly(module, &reg_map, out);
	reg_map_clear(&reg_map);

	fclose(out);
	LLVMDisposeModule(module);
	LLVMShutdown();
	return 0;
}
