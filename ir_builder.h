#ifndef IR_BUILDER_H
#define IR_BUILDER_H

#include "frontend/ast/ast.h"
#include <llvm-c/Core.h>

// IR Builder function
// Input: AST root node
// Output: LLVM module reference
LLVMModuleRef buildIR(astNode* root);

#endif // IR_BUILDER_H
