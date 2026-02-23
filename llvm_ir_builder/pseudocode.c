#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <map>
#include "ast/ast.h"
#include <llvm-c/Core.h>

#void builder(astNode* root)
#   create a module with LLVMModuleCreateWithName() using file name
#   set final architecture target to x86_64-pc-linux-gnu for the module
#   add read type function ref
#       create empty param ref for read function
#       create read function type
#       finally create function ref
#   add print type function ref
#       create param ref with LLVMInt32Type
#       create print function type
#       finally create print function ref
#   create builder using LLVMCreateBuilder()
#   initialize variable std::map<std::string, LLVMValueRef> varMap
#   initialize global variable counter std::map<std::string, int> globalCounter;
#   get the function node from root (root->prog.func)
#   call preprocessVars() with builder, varMap, globalCounter, and function body to rename variables
#   create the main function in LLVM
#       get function name from function node (func->func.name)
#       create function type with LLVMInt32Type as return type and empty param types
#       create function using LLVMAddFunction() with module, function name, and function type
#       get the first basic block of the function using LLVMGetEntryBasicBlock()
#       position builder at the end of first basic block using LLVMPositionBuilderAtEnd()
#   in the first basic block, create alloc instructions for all variables
#       if function has a parameter (func->func.param != NULL)
#           get parameter name from param node (param->var.name)
#           create allocation using LLVMBuildAlloca() with LLVMInt32Type() and parameter name, assign to paramAlloca
#           set memory alignment for the allocation with LLVMSetAlignment() to 4
#           store parameter value into paramAlloca using LLVMBuildStore() with function parameter value and paramAlloca
#           set key: parameter name -> value: paramAlloca in the varMap
#       iterate through varMap to get all local variable names and their alloc refs
#           (alloc instructions for local variables were already created in preprocessVars)
#           (varMap already contains the mapping from variable name to alloc instruction)
#       create allocation for return value using LLVMBuildAlloca() with LLVMInt32Type() and name "ret_val", assign to retAlloca
#       set memory alignment for the allocation with LLVMSetAlignment() to 4
#       store retAlloca in varMap with key "ret_val" or keep as global variable
#   create special return basic block for the function
#       get current function from builder
#       create return_block basic block using LLVMAppendBasicBlock() with function and name "return_block"
#       position builder at end of return_block using LLVMPositionBuilderAtEnd()
#       load value from retAlloca using LLVMBuildLoad() with builder, LLVMInt32Type(), and retAlloca
#       create return instruction using LLVMBuildRet() with builder and loaded value
#   traverse the AST to build IR by calling createBlockRef() with builder, function body, varMap, retAlloca, and return_block
#   dispose builder using LLVMDisposeBuilder()


#LLVMValueRef createBlockRef(LLVMBuilderRef builder, astNode* node, std::map<std::string, LLVMValueRef> varMap, LLVMValueRef retAlloca, LLVMBasicBlockRef returnBlock)
#   if node is NULL or node type is not ast_stmt or stmt type is not ast_block, return NULL
#   get the statement list from node (node->stmt.block.stmt_list)
#   iterate through each statement in the statement list
#       if statement is a declaration (ast_decl)
#           (alloc instruction already created in preprocessVars, skip)
#       elif statement is an assignment (ast_asgn)
#           get lhs node (stmt->stmt.asgn.lhs)
#           get rhs node (stmt->stmt.asgn.rhs)
#           create term ref for rhs by calling createTermRef() with builder, rhs node, and varMap
#           get variable name from lhs (lhs->var.name)
#           get alloca ref from varMap using variable name
#           store the rhs term ref into the alloca using LLVMBuildStore() with builder, rhs ref, and alloca ref
#       elif statement is a return (ast_ret)
#           get expression node from return (stmt->stmt.ret.expr)
#           create term ref for expression by calling createTermRef() with builder, expr node, and varMap
#           store the expression ref into retAlloca using LLVMBuildStore() with builder, expr ref, and retAlloca
#           create branch to return_block using LLVMBuildBr() with builder and returnBlock
#       elif statement is a call (ast_call)
#           get function name from statement (stmt->stmt.call.name)
#           if function name is "print"
#               get parameter node from call (stmt->stmt.call.param)
#               create term ref for parameter by calling createTermRef() with builder, param node, and varMap
#               get print function ref from module
#               create call instruction using LLVMBuildCall() with builder, print function type, print function ref, param ref, and name "print"
#           elif function name is "read"
#               get read function ref from module
#               create call instruction using LLVMBuildCall() with builder, read function type, read function ref, empty params, and name "read"
#               (read returns a value that can be used in assignments)
#       elif statement is a block (ast_block)
#           recursively call createBlockRef() with builder, block node, varMap, retAlloca, and returnBlock
#       elif statement is a while loop (ast_while)
#           get condition node (stmt->stmt.whilen.cond)
#           get body node (stmt->stmt.whilen.body)
#           get current function from builder
#           create while_cond basic block using LLVMAppendBasicBlock() with function and name "while_cond"
#           create while_body basic block using LLVMAppendBasicBlock() with function and name "while_body"
#           create while_end basic block using LLVMAppendBasicBlock() with function and name "while_end"
#           create branch to while_cond using LLVMBuildBr() with builder and while_cond block
#           position builder at end of while_cond block using LLVMPositionBuilderAtEnd()
#           create term ref for condition by calling createTermRef() with builder, cond node, and varMap
#           create conditional branch using LLVMBuildCondBr() with builder, cond ref, while_body block, and while_end block
#           position builder at end of while_body block using LLVMPositionBuilderAtEnd()
#           recursively call createBlockRef() with builder, body node, varMap, retAlloca, and returnBlock
#           create branch back to while_cond using LLVMBuildBr() with builder and while_cond block
#           (while_cond is the exit point for the while body - body branches back to condition check)
#           position builder at end of while_end block using LLVMPositionBuilderAtEnd()
#       elif statement is an if statement (ast_if)
#           get condition node (stmt->stmt.ifn.cond)
#           get if body node (stmt->stmt.ifn.if_body)
#           get else body node (stmt->stmt.ifn.else_body) - may be NULL
#           get current function from builder
#           create if_entry basic block using LLVMAppendBasicBlock() with function and name "if_entry"
#           create if_then basic block using LLVMAppendBasicBlock() with function and name "if_then"
#           create if_else basic block using LLVMAppendBasicBlock() with function and name "if_else" (if else body exists)
#           create if_exit basic block using LLVMAppendBasicBlock() with function and name "if_exit"
#           create branch to if_entry using LLVMBuildBr() with builder and if_entry block
#           position builder at end of if_entry block using LLVMPositionBuilderAtEnd()
#           create term ref for condition by calling createTermRef() with builder, cond node, and varMap
#           (if_entry is the entry basic block that checks the condition)
#           if else body exists
#               create conditional branch using LLVMBuildCondBr() with builder, cond ref, if_then block, and if_else block
#           else
#               create conditional branch using LLVMBuildCondBr() with builder, cond ref, if_then block, and if_exit block
#           position builder at end of if_then block using LLVMPositionBuilderAtEnd()
#           recursively call createBlockRef() with builder, if_body node, varMap, retAlloca, and returnBlock
#           create branch to if_exit using LLVMBuildBr() with builder and if_exit block
#           (if_exit is the exit basic block - destination for branches from if and else bodies)
#           if else body exists
#               position builder at end of if_else block using LLVMPositionBuilderAtEnd()
#               recursively call createBlockRef() with builder, else_body node, varMap, retAlloca, and returnBlock
#               create branch to if_exit using LLVMBuildBr() with builder and if_exit block
#           position builder at end of if_exit block using LLVMPositionBuilderAtEnd()



#LLVMValueRef createRelExprRef(LLVMBuilderRef builder, astNode* node, std::map<std::string, LLVMValueRef> varMap)
#   if node is NULL or node type is not ast_rexpr, return NULL
#   get lhs node (node->rexpr.lhs)
#   get rhs node (node->rexpr.rhs)
#   create term ref for lhs by calling createTermRef() with builder, lhs node, and varMap, assign to l1
#   create term ref for rhs by calling createTermRef() with builder, rhs node, and varMap, assign to l2
#   extract relational operator type from node (node->rexpr.op)
#   determine LLVMIntPredicate based on rop_type
#       if rop_type is lt, use LLVMIntSLT
#       elif rop_type is gt, use LLVMIntSGT
#       elif rop_type is le, use LLVMIntSLE
#       elif rop_type is ge, use LLVMIntSGE
#       elif rop_type is eq, use LLVMIntEQ
#       elif rop_type is neq, use LLVMIntNE
#   create ICmp instruction using LLVMBuildICmp() with builder, predicate, l1, l2, and name "cmp"
#   return the ICmp ref


#LLVMValueRef createArithExprRef(LLVMBuilderRef builder, astNode* node, std::map<std::string, LLVMValueRef> varMap)
#   if node is NULL, return NULL
#   if node type is ast_uexpr (unary expression)
#       get expression node (node->uexpr.expr)
#       get operator type (node->uexpr.op)
#       if operator is uminus
#           create term ref for expression by calling createTermRef() with builder, expr node, and varMap
#           create constant zero using LLVMConstInt() with LLVMInt32Type(), 0, and sign-extend true
#           create subtraction using LLVMBuildSub() with builder, zero constant, expr ref, and name "neg"
#           return the subtraction ref
#   elif node type is ast_bexpr (binary expression)
#       get lhs node (node->bexpr.lhs)
#       get rhs node (node->bexpr.rhs)
#       create term ref for lhs by calling createTermRef() with builder, lhs node, and varMap, assign to l1
#       create term ref for rhs by calling createTermRef() with builder, rhs node, and varMap, assign to l2
#       extract operator type from node (node->bexpr.op)
#       if op type is addition
#           create add instruction using LLVMBuildAdd() with builder, l1, l2, and name "add"
#           return the add ref
#       elif op type is subtraction
#           create sub instruction using LLVMBuildSub() with builder, l1, l2, and name "sub"
#           return the sub ref
#       elif op type is multiplication
#           create mul instruction using LLVMBuildMul() with builder, l1, l2, and name "mul"
#           return the mul ref
#       elif op type is divide
#           create sdiv instruction using LLVMBuildSDiv() with builder, l1, l2, and name "div"
#           return the div ref


#LLVMValueRef createTermRef(LLVMBuilderRef builder, astNode* node, std::map<std::string, LLVMValueRef> varMap)
#   if node is NULL, return NULL
#   if node type is ast_cnst (constant)
#       get constant value from node (node->cnst.value)
#       create constant integer using LLVMConstInt() with LLVMInt32Type(), value, and sign-extend true
#       return the constant ref
#   elif node type is ast_var (variable)
#       get variable name from node (node->var.name)
#       get alloca ref from varMap using variable name
#       if alloca ref is NULL, return NULL (variable not found)
#       load value from alloca using LLVMBuildLoad() with builder, LLVMInt32Type(), alloca ref, and variable name
#       return the load ref
#   elif node type is ast_rexpr (relational expression)
#       call createRelExprRef() with builder, node, and varMap, and return the result
#   elif node type is ast_bexpr or ast_uexpr (arithmetic expression)
#       call createArithExprRef() with builder, node, and varMap, and return the result


#void renameVarInNode(astNode* node, const char* oldName, const char* newName)
#   if node is NULL, return
#   check node type
#       if node is a variable (ast_var)
#           if node->var.name equals oldName
#               free old name and set node->var.name to strdup(newName)
#       elif node is a relational expression (ast_rexpr)
#           recursively call renameVarInNode() on node->rexpr.lhs with oldName and newName
#           recursively call renameVarInNode() on node->rexpr.rhs with oldName and newName
#       elif node is a binary expression (ast_bexpr)
#           recursively call renameVarInNode() on node->bexpr.lhs with oldName and newName
#           recursively call renameVarInNode() on node->bexpr.rhs with oldName and newName
#       elif node is a unary expression (ast_uexpr)
#           recursively call renameVarInNode() on node->uexpr.expr with oldName and newName
#       elif node is a statement (ast_stmt)
#           check statement type
#               if statement is assignment (ast_asgn)
#                   recursively call renameVarInNode() on node->stmt.asgn.lhs with oldName and newName
#                   recursively call renameVarInNode() on node->stmt.asgn.rhs with oldName and newName
#               elif statement is call (ast_call)
#                   if node->stmt.call.param is not NULL
#                       recursively call renameVarInNode() on node->stmt.call.param with oldName and newName
#               elif statement is return (ast_ret)
#                   recursively call renameVarInNode() on node->stmt.ret.expr with oldName and newName


#void preprocessVars(LLVMBuilderRef builder, std::map<std::string, LLVMValueRef> varMap, std::map<std::string, int> globalCounter, astNode* scope)
#   get the statement list from scope (scope->stmt.block.stmt_list)
#   iterate through each statement in the statement list
#       if statement is a declaration (ast_decl)
#           get the variable name from statement (stmt->stmt.decl.name)
#           check if name exists in globalCounter
#           if name already exists in globalCounter
#               get current count from globalCounter
#               increase counter by 1 and update globalCounter
#               create new name by concatenating original name with count (e.g., "name" + count -> "name1")
#               free old name and set stmt->stmt.decl.name to new name
#               create allocation using LLVMBuildAlloca() with LLVMInt32Type() and new name, assign to ref
#               set memory alignment for the allocation with LLVMSetAlignment() to 4
#               set key: new var name -> value: ref in the varMap
#               iterate through all subsequent statements in the statement list
#                   for each subsequent statement, recursively rename all uses of original name to new name
#           else
#               add var to globalCounter with count = 1
#               create allocation using LLVMBuildAlloca() with LLVMInt32Type() and original name, assign to ref
#               set memory alignment for the allocation with LLVMSetAlignment() to 4
#               set key: var name -> value: ref in the varMap
#       elif statement is an assignment (ast_asgn)
#           get lhs node from statement (stmt->stmt.asgn.lhs)
#           get rhs node from statement (stmt->stmt.asgn.rhs)
#           (variable renaming in assignments is handled when processing declarations)
#       elif statement is a block (ast_block)
#           recursively call preprocessVars() with builder, varMap, globalCounter, and the block statement node
#       elif statement is a while loop (ast_while)
#           get the body node from statement (stmt->stmt.whilen.body)
#           recursively call preprocessVars() on the body with builder, varMap, globalCounter, and body node
#       elif statement is an if statement (ast_if)
#           get the if body node from statement (stmt->stmt.ifn.if_body)
#           recursively call preprocessVars() on the if body with builder, varMap, globalCounter, and if_body node
#           if there is an else body (stmt->stmt.ifn.else_body != NULL)
#               get the else body node from statement (stmt->stmt.ifn.else_body)
#               recursively call preprocessVars() on the else body with builder, varMap, globalCounter, and else_body node


