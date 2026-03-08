#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <string>
#include <map>
#include <vector>
#include "../frontend/ast/ast.h"
#include "../ir_builder.h"
#include <llvm-c/Core.h>

static void renameVarInNode(astNode* node, const char* oldName, const char* newName);
static void preprocessVars(LLVMBuilderRef builder, std::map<std::string, LLVMValueRef>& varMap, 
                          std::map<std::string, int>& globalCounter, astNode* scope);
static LLVMValueRef createBlockRef(LLVMBuilderRef builder, astNode* node, 
                                   std::map<std::string, LLVMValueRef>& varMap, 
                                   LLVMValueRef retAlloca, LLVMBasicBlockRef returnBlock,
                                   LLVMValueRef printFunc, LLVMValueRef readFunc);
static LLVMValueRef createRelExprRef(LLVMBuilderRef builder, astNode* node, 
                                     std::map<std::string, LLVMValueRef>& varMap);
static LLVMValueRef createArithExprRef(LLVMBuilderRef builder, astNode* node, 
                                       std::map<std::string, LLVMValueRef>& varMap);
static LLVMValueRef createTermRef(LLVMBuilderRef builder, astNode* node, 
                                  std::map<std::string, LLVMValueRef>& varMap);


static void renameVarInNode(astNode* node, const char* oldName, const char* newName) {
    if (node == NULL) return;
    
    switch (node->type) {
        case ast_var: {
            if (node->var.name != NULL && strcmp(node->var.name, oldName) == 0) {
                free(node->var.name);
                node->var.name = strdup(newName);
            }
            break;
        }
        case ast_rexpr: {
            renameVarInNode(node->rexpr.lhs, oldName, newName);
            renameVarInNode(node->rexpr.rhs, oldName, newName);
            break;
        }
        case ast_bexpr: {
            renameVarInNode(node->bexpr.lhs, oldName, newName);
            renameVarInNode(node->bexpr.rhs, oldName, newName);
            break;
        }
        case ast_uexpr: {
            renameVarInNode(node->uexpr.expr, oldName, newName);
            break;
        }
        case ast_stmt: {
            switch (node->stmt.type) {
                case ast_asgn: {
                    renameVarInNode(node->stmt.asgn.lhs, oldName, newName);
                    renameVarInNode(node->stmt.asgn.rhs, oldName, newName);
                    break;
                }
                case ast_call: {
                    if (node->stmt.call.param != NULL) {
                        renameVarInNode(node->stmt.call.param, oldName, newName);
                    }
                    break;
                }
                case ast_ret: {
                    renameVarInNode(node->stmt.ret.expr, oldName, newName);
                    break;
                }
                default:
                    break;
            }
            break;
        }
        default:
            break;
    }
}

// returns new variable name
static char* createRenamedVar(const char *name, int count) {
    char *newName = (char*)malloc(strlen(name) + 20);
    sprintf(newName, "%s%d", name, count);
    return newName;
}

static void preprocessVars(LLVMBuilderRef builder, std::map<std::string, LLVMValueRef>& varMap, 
                          std::map<std::string, int>& globalCounter, astNode* scope) {
    if (scope == NULL || scope->type != ast_stmt || scope->stmt.type != ast_block) {
        return;
    }
    
    std::vector<astNode*> *stmt_list = scope->stmt.block.stmt_list;
    if (stmt_list == NULL) {
        return;
    }
    
    std::vector<astNode*>::iterator it = stmt_list->begin();
    while (it != stmt_list->end()) {
        astNode *stmt = *it;
        
        if (stmt == NULL || stmt->type != ast_stmt) {
            it++;
            continue;
        }
        
        switch (stmt->stmt.type) {
            case ast_decl: {
                const char *varName = stmt->stmt.decl.name;
                
                if (varName == NULL) {
                    it++;
                    break;
                }
                
                // check if variable name already exists in globalCounter
                int count = globalCounter[varName];
                char *newVarName;
                
                if (count > 0) {
                    // variable name already exists, increment counter and rename
                    globalCounter[varName] = count + 1;
                    newVarName = createRenamedVar(varName, count);
                    free(stmt->stmt.decl.name);
                    stmt->stmt.decl.name = newVarName;
                } else {
                    globalCounter[varName] = 1;
                    newVarName = strdup(varName);
                }
                
                LLVMTypeRef intType = LLVMInt32Type();
                LLVMValueRef alloca = LLVMBuildAlloca(builder, intType, newVarName);
                
                varMap[std::string(newVarName)] = alloca;
                
                std::vector<astNode*>::iterator renameIt = it;
                renameIt++; 
                while (renameIt != stmt_list->end()) {
                    renameVarInNode(*renameIt, varName, newVarName);
                    renameIt++;
                }
                
                break;
            }
            
            case ast_asgn: {
                break;
            }
            
            case ast_block: {
                // recursively process nested block
                preprocessVars(builder, varMap, globalCounter, stmt);
                break;
            }
            
            case ast_while: {
                if (stmt->stmt.whilen.body != NULL) {
                    preprocessVars(builder, varMap, globalCounter, stmt->stmt.whilen.body);
                }
                break;
            }
            
            case ast_if: {
                if (stmt->stmt.ifn.if_body != NULL) {
                    preprocessVars(builder, varMap, globalCounter, stmt->stmt.ifn.if_body);
                }
                if (stmt->stmt.ifn.else_body != NULL) {
                    preprocessVars(builder, varMap, globalCounter, stmt->stmt.ifn.else_body);
                }
                break;
            }
            
            default:
                break;
        }
        
        it++;
    }
}

// create term reference (constant, variable, or expression)
static LLVMValueRef createTermRef(LLVMBuilderRef builder, astNode* node, 
                                  std::map<std::string, LLVMValueRef>& varMap) {
    if (node == NULL) return NULL;
    
    switch (node->type) {
        case ast_cnst: {
            int value = node->cnst.value;
            LLVMValueRef constRef = LLVMConstInt(LLVMInt32Type(), value, 1); // sign-extend = true
            return constRef;
        }
        
        case ast_var: {
            const char *varName = node->var.name;
            if (varName == NULL) return NULL;
            
            if (varMap.find(std::string(varName)) == varMap.end()) {
                return NULL; 
            }
            LLVMValueRef allocaRef = varMap[std::string(varName)];
            
            LLVMValueRef loadRef = LLVMBuildLoad2(builder, LLVMInt32Type(), allocaRef, varName);
            return loadRef;
        }
        
        case ast_rexpr: {
            return createRelExprRef(builder, node, varMap);
        }
        
        case ast_bexpr:
        case ast_uexpr: {
            return createArithExprRef(builder, node, varMap);
        }
        
        default:
            return NULL;
    }
}

static LLVMValueRef createRelExprRef(LLVMBuilderRef builder, astNode* node, 
                                    std::map<std::string, LLVMValueRef>& varMap) {
    if (node == NULL || node->type != ast_rexpr) return NULL;
    
    astNode *lhs = node->rexpr.lhs;
    astNode *rhs = node->rexpr.rhs;
    
    LLVMValueRef l1 = createTermRef(builder, lhs, varMap);
    LLVMValueRef l2 = createTermRef(builder, rhs, varMap);
    
    if (l1 == NULL || l2 == NULL) return NULL;
    
    rop_type op = node->rexpr.op;
    
    LLVMIntPredicate predicate;
    switch (op) {
        case lt:  predicate = LLVMIntSLT; break;
        case gt:  predicate = LLVMIntSGT; break;
        case le:  predicate = LLVMIntSLE; break;
        case ge:  predicate = LLVMIntSGE; break;
        case eq:  predicate = LLVMIntEQ; break;
        case neq: predicate = LLVMIntNE; break;
        default:  return NULL;
    }
    
    LLVMValueRef icmpRef = LLVMBuildICmp(builder, predicate, l1, l2, "cmp");
    return icmpRef;
}

static LLVMValueRef createArithExprRef(LLVMBuilderRef builder, astNode* node, 
                                      std::map<std::string, LLVMValueRef>& varMap) {
    if (node == NULL) return NULL;
    
    if (node->type == ast_uexpr) {
        astNode *expr = node->uexpr.expr;
        op_type op = node->uexpr.op;
        
        if (op == uminus) {
            LLVMValueRef exprRef = createTermRef(builder, expr, varMap);
            if (exprRef == NULL) return NULL;
            
            LLVMValueRef zero = LLVMConstInt(LLVMInt32Type(), 0, 1);
            
            LLVMValueRef subRef = LLVMBuildSub(builder, zero, exprRef, "neg");
            return subRef;
        }
    } else if (node->type == ast_bexpr) {
        astNode *lhs = node->bexpr.lhs;
        astNode *rhs = node->bexpr.rhs;
        
        LLVMValueRef l1 = createTermRef(builder, lhs, varMap);
        LLVMValueRef l2 = createTermRef(builder, rhs, varMap);
        
        if (l1 == NULL || l2 == NULL) return NULL;
        
        op_type op = node->bexpr.op;
        
        switch (op) {
            case add:
                return LLVMBuildAdd(builder, l1, l2, "add");
            case sub:
                return LLVMBuildSub(builder, l1, l2, "sub");
            case mul:
                return LLVMBuildMul(builder, l1, l2, "mul");
            case divide:
                return LLVMBuildSDiv(builder, l1, l2, "div");
            default:
                return NULL;
        }
    }
    
    return NULL;
}

static LLVMValueRef createBlockRef(LLVMBuilderRef builder, astNode* node, 
                                  std::map<std::string, LLVMValueRef>& varMap, 
                                  LLVMValueRef retAlloca, LLVMBasicBlockRef returnBlock,
                                  LLVMValueRef printFunc, LLVMValueRef readFunc) {
    if (node == NULL || node->type != ast_stmt || node->stmt.type != ast_block) {
        return NULL;
    }
    
    std::vector<astNode*> *stmt_list = node->stmt.block.stmt_list;
    if (stmt_list == NULL) {
        return NULL;
    }
    
    std::vector<astNode*>::iterator it = stmt_list->begin();
    while (it != stmt_list->end()) {
        astNode *stmt = *it;
        
        if (stmt == NULL || stmt->type != ast_stmt) {
            it++;
            continue;
        }
        
        switch (stmt->stmt.type) {
            case ast_decl: {
                // Alloc instruction already created in preprocessVars, skip
                break;
            }
            
            case ast_asgn: {
                astNode *lhs = stmt->stmt.asgn.lhs;
                astNode *rhs = stmt->stmt.asgn.rhs;
                
                LLVMValueRef rhsRef = createTermRef(builder, rhs, varMap);
                if (rhsRef == NULL) {
                    it++;
                    break;
                }
                
                if (lhs == NULL || lhs->type != ast_var) {
                    it++;
                    break;
                }
                const char *varName = lhs->var.name;
                
                if (varMap.find(std::string(varName)) == varMap.end()) {
                    it++;
                    break;
                }
                LLVMValueRef allocaRef = varMap[std::string(varName)];
                
                LLVMBuildStore(builder, rhsRef, allocaRef);
                break;
            }
            
            case ast_ret: {
                astNode *expr = stmt->stmt.ret.expr;
                
                LLVMValueRef exprRef = createTermRef(builder, expr, varMap);
                if (exprRef == NULL) {
                    it++;
                    break;
                }
                
                LLVMBuildStore(builder, exprRef, retAlloca);
                
                LLVMBuildBr(builder, returnBlock);
                break;
            }
            
            case ast_call: {
                const char *funcName = stmt->stmt.call.name;
                
                if (funcName != NULL && strcmp(funcName, "print") == 0) {
                    astNode *param = stmt->stmt.call.param;
                    if (param != NULL) {
                        LLVMValueRef paramRef = createTermRef(builder, param, varMap);
                        if (paramRef != NULL && printFunc != NULL) {
                            LLVMValueRef args[] = {paramRef};
                            LLVMTypeRef printFuncType = LLVMGlobalGetValueType(printFunc);
                            LLVMBuildCall2(builder, printFuncType, printFunc, args, 1, "print");
                        }
                    }
                } else if (funcName != NULL && strcmp(funcName, "read") == 0) {
                    if (readFunc != NULL) {
                        LLVMTypeRef readFuncType = LLVMGlobalGetValueType(readFunc);
                        LLVMBuildCall2(builder, readFuncType, readFunc, NULL, 0, "read");
                    }
                }
                break;
            }
            
            case ast_block: {
                createBlockRef(builder, stmt, varMap, retAlloca, returnBlock, printFunc, readFunc);
                break;
            }
            
            case ast_while: {
                astNode *cond = stmt->stmt.whilen.cond;
                astNode *body = stmt->stmt.whilen.body;
                
                LLVMBasicBlockRef currentBB = LLVMGetInsertBlock(builder);
                LLVMValueRef function = LLVMGetBasicBlockParent(currentBB);

                LLVMBasicBlockRef while_cond = LLVMAppendBasicBlock(function, "while_cond");
                LLVMBasicBlockRef while_body = LLVMAppendBasicBlock(function, "while_body");
                LLVMBasicBlockRef while_end = LLVMAppendBasicBlock(function, "while_end");
                
                LLVMBuildBr(builder, while_cond);

                LLVMPositionBuilderAtEnd(builder, while_cond);
                
                LLVMValueRef condRef = createTermRef(builder, cond, varMap);
                if (condRef != NULL) {
                    LLVMBuildCondBr(builder, condRef, while_body, while_end);
                }
                
                LLVMPositionBuilderAtEnd(builder, while_body);
                
                createBlockRef(builder, body, varMap, retAlloca, returnBlock, printFunc, readFunc);
                
                LLVMBuildBr(builder, while_cond);
                
                LLVMPositionBuilderAtEnd(builder, while_end);
                break;
            }
            
            case ast_if: {

                astNode *cond = stmt->stmt.ifn.cond;
                astNode *if_body = stmt->stmt.ifn.if_body;
                astNode *else_body = stmt->stmt.ifn.else_body;
                
                LLVMBasicBlockRef currentBB = LLVMGetInsertBlock(builder);
                LLVMValueRef function = LLVMGetBasicBlockParent(currentBB);
                
                LLVMBasicBlockRef if_entry = LLVMAppendBasicBlock(function, "if_entry");
                LLVMBasicBlockRef if_then = LLVMAppendBasicBlock(function, "if_then");
                LLVMBasicBlockRef if_else = NULL;
                LLVMBasicBlockRef if_exit = LLVMAppendBasicBlock(function, "if_exit");
                
                if (else_body != NULL) {
                    if_else = LLVMAppendBasicBlock(function, "if_else");
                }
                
                LLVMBuildBr(builder, if_entry);
                
                LLVMPositionBuilderAtEnd(builder, if_entry);
                
                LLVMValueRef condRef = createTermRef(builder, cond, varMap);
                if (condRef != NULL) {
                    if (else_body != NULL) {
                        LLVMBuildCondBr(builder, condRef, if_then, if_else);
                    } else {
                        LLVMBuildCondBr(builder, condRef, if_then, if_exit);
                    }
                }
                
                LLVMPositionBuilderAtEnd(builder, if_then);
                
                createBlockRef(builder, if_body, varMap, retAlloca, returnBlock, printFunc, readFunc);
                
                LLVMBuildBr(builder, if_exit);
                
                if (else_body != NULL) {
                    LLVMPositionBuilderAtEnd(builder, if_else);
                    
                    createBlockRef(builder, else_body, varMap, retAlloca, returnBlock, printFunc, readFunc);
                    
                    LLVMBuildBr(builder, if_exit);
                }
                
                LLVMPositionBuilderAtEnd(builder, if_exit);
                break;
            }
            
            default:
                break;
        }
        
        it++;
    }
    
    return NULL;
}

// Main IR builder function
LLVMModuleRef buildIR(astNode* root) {
    if (root == NULL || root->type != ast_prog) {
        return NULL;
    }
    
    LLVMModuleRef module = LLVMModuleCreateWithName("miniC_module");
    
    // Note: Not setting target/data layout - following day5 example pattern
    // LLVMSetTarget(module, "x86_64-pc-linux-gnu");
    
    LLVMTypeRef readParamTypes[] = {};
    LLVMTypeRef readFuncType = LLVMFunctionType(LLVMInt32Type(), readParamTypes, 0, 0);
    LLVMValueRef readFunc = LLVMAddFunction(module, "read", readFuncType);
    
    LLVMTypeRef printParamTypes[] = {LLVMInt32Type()};
    LLVMTypeRef printFuncType = LLVMFunctionType(LLVMVoidType(), printParamTypes, 1, 0);
    LLVMValueRef printFunc = LLVMAddFunction(module, "print", printFuncType);
    
    LLVMBuilderRef builder = LLVMCreateBuilder();
    
    // initialize variable maps
    std::map<std::string, LLVMValueRef> varMap;
    std::map<std::string, int> globalCounter;
    
    astNode *func = root->prog.func;
    if (func == NULL || func->type != ast_func) {
        LLVMDisposeBuilder(builder);
        return NULL;
    }
    
    const char *funcName = func->func.name;
    if (funcName == NULL) {
        funcName = "main";
    }
    
    LLVMTypeRef paramTypes[1];
    unsigned numParams = 0;
    if (func->func.param != NULL) {
        paramTypes[0] = LLVMInt32Type();
        numParams = 1;
    }
    
    LLVMTypeRef funcType = LLVMFunctionType(LLVMInt32Type(), 
                                            (numParams > 0) ? paramTypes : NULL, 
                                            numParams, 0);
    
    LLVMValueRef llvmFunc = LLVMAddFunction(module, funcName, funcType);
    
    // Create entry basic block explicitly (like day5 example)
    LLVMBasicBlockRef entryBB = LLVMAppendBasicBlock(llvmFunc, "entry");
    
    LLVMPositionBuilderAtEnd(builder, entryBB);
    
    // IMPORTANT: Create parameter alloca FIRST, before preprocessVars
    // This ensures parameter is in varMap before any variable processing
    if (func->func.param != NULL && func->func.param->type == ast_var) {
        const char *paramName = func->func.param->var.name;
        if (paramName != NULL) {
            // create allocation for parameter
            LLVMValueRef paramAlloca = LLVMBuildAlloca(builder, LLVMInt32Type(), paramName);
            // LLVMSetAlignment(paramAlloca, 4);
            
            LLVMValueRef paramValue = LLVMGetParam(llvmFunc, 0);
            
            LLVMBuildStore(builder, paramValue, paramAlloca);
            
            // set key: parameter name -> value: paramAlloca in the varMap
            varMap[std::string(paramName)] = paramAlloca;
        }
    }
    
    // Preprocess variables (rename and create allocas) - must be done after function is created
    if (func->func.body != NULL) {
        preprocessVars(builder, varMap, globalCounter, func->func.body);
    }
    
    LLVMValueRef retAlloca = LLVMBuildAlloca(builder, LLVMInt32Type(), "ret_val");
    // LLVMSetAlignment(retAlloca, 4);
    
    LLVMBasicBlockRef returnBlock = LLVMAppendBasicBlock(llvmFunc, "return_block");
    
    LLVMPositionBuilderAtEnd(builder, returnBlock);
    
    LLVMValueRef retValue = LLVMBuildLoad2(builder, LLVMInt32Type(), retAlloca, "ret_val");
    
    LLVMBuildRet(builder, retValue);
    
    LLVMPositionBuilderAtEnd(builder, entryBB);
    
    if (func->func.body != NULL) {
        createBlockRef(builder, func->func.body, varMap, retAlloca, returnBlock, printFunc, readFunc);
    }
    
    LLVMDisposeBuilder(builder);
    
    return module;
}
