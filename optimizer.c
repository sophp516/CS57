#include "optimizer.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <llvm-c/Core.h>

// Forward declaration from optimizations/llvm_parser.c
extern void walkFunctions(LLVMModuleRef module);

// Optimize the entire module
// This function wraps the walkFunctions from llvm_parser.c
// which performs all optimizations (constant propagation, constant folding, 
// dead code elimination, common subexpression elimination)
LLVMModuleRef optimizeModule(LLVMModuleRef module) {
    if (module == NULL) {
        return NULL;
    }
    
    // Apply all optimizations to the module
    walkFunctions(module);
    
    return module;
}
