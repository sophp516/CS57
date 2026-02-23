#ifndef OPTIMIZER_H
#define OPTIMIZER_H

#include <llvm-c/Core.h>

// Optimizer function
// Input: LLVM module reference
// Output: Optimized LLVM module reference (or same module if optimization fails)
LLVMModuleRef optimizeModule(LLVMModuleRef module);

#endif // OPTIMIZER_H
