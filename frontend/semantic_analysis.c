#include "symantic_analysis.h"

std::vector<std::vector<std::string>> symbolTable;

bool varExists(std::string varName)
{
    for (int i = symbolTable.size() - 1; i >= 0; i--) 
    {   
        const std::vector<std::string>& currScope = symbolTable[i];
        for (int j = currScope.size() - 1; j >= 0; j--) 
        {
            if (currScope[j] == varName) return true;
        }
    }
    return false;
}

bool addVarToScope(std::string varName)
{
    if (symbolTable.empty()) return false;

    // Check if variable already exists in current scope (duplicate in same scope is not allowed)
    std::vector<std::string>& currScope = symbolTable.back();
    for (size_t i = 0; i < currScope.size(); i++) {
        if (currScope[i] == varName) {
            return false; // Duplicate in same scope
        }
    }
    
    // Note: Variable shadowing (declaring same name in nested scope) is allowed
    // So we don't check parent scopes - shadowing is permitted
    
    currScope.push_back(varName);
    return true;
}

bool pushScope()
{
    symbolTable.push_back(std::vector<std::string>());
    return true;
}

bool popScope()
{
    if (symbolTable.empty()) return false;
    symbolTable.pop_back();
    return true;
}

bool checkProg(astNode* root)
{
    if (root == NULL || root->type != ast_prog) {
        fprintf(stderr, "DEBUG: checkProg: root is NULL or not ast_prog\n");
        return false;
    }
    
    bool result = checkFunc(root->prog.func);
    if (!result) {
        fprintf(stderr, "DEBUG: checkProg: checkFunc failed\n");
    }
    return result;
}

bool checkFunc(astNode* func)
{
    if (func == NULL || func->type != ast_func) {
        fprintf(stderr, "DEBUG: checkFunc: func is NULL or not ast_func\n");
        return false;
    }
    
    pushScope();
    
    if (func->func.param != NULL) {
        if (func->func.param->type == ast_var) {
            std::string paramName(func->func.param->var.name);
            if (!addVarToScope(paramName)) {
                fprintf(stderr, "DEBUG: checkFunc: failed to add param %s to scope\n", paramName.c_str());
                popScope();
                return false;
            }
        }
    }
    
    bool result = checkBlock(func->func.body);
    if (!result) {
        fprintf(stderr, "DEBUG: checkFunc: checkBlock failed\n");
    }
    
    popScope();
    
    return result;
}

bool checkBlock(astNode* block)
{
    if (block == NULL || block->type != ast_stmt || block->stmt.type != ast_block) {
        fprintf(stderr, "DEBUG: checkBlock: block is NULL or not ast_block\n");
        return false;
    }
    
    pushScope();
    
    std::vector<astNode*>* stmt_list = block->stmt.block.stmt_list;
    if (stmt_list == NULL) {
        popScope();
        return true;
    }
    
    // First pass: process all declarations
    for (size_t i = 0; i < stmt_list->size(); i++) {
        astNode* node = (*stmt_list)[i];
        if (node == NULL) continue;
        
        if (node->type == ast_stmt && node->stmt.type == ast_decl) {
            std::string varName(node->stmt.decl.name);
            if (!addVarToScope(varName)) {
                fprintf(stderr, "DEBUG: checkBlock: failed to add var %s to scope (duplicate?)\n", varName.c_str());
                popScope();
                return false; // Duplicate declaration
            }
        }
    }
    
    // Second pass: check all statements (declarations already processed, now check uses)
    for (size_t i = 0; i < stmt_list->size(); i++) {
        astNode* node = (*stmt_list)[i];
        if (node == NULL) continue;
        
        if (!checkStmt(node)) {
            popScope();
            return false;
        }
    }
    
    // Pop block scope
    popScope();
    return true;
}

bool checkStmt(astNode* stmt)
{
    if (stmt == NULL || stmt->type != ast_stmt) return false;
    
    switch (stmt->stmt.type) {
        case ast_decl:
            // Already handled in checkBlock's first pass
            return true;
            
        case ast_asgn: {
            // Check lhs (variable being assigned to) and rhs (expression)
            if (stmt->stmt.asgn.lhs == NULL || stmt->stmt.asgn.rhs == NULL) {
                fprintf(stderr, "DEBUG: checkStmt ast_asgn: lhs or rhs is NULL\n");
                return false;
            }
            
            // Check lhs is a variable
            if (stmt->stmt.asgn.lhs->type != ast_var) {
                fprintf(stderr, "DEBUG: checkStmt ast_asgn: lhs is not ast_var\n");
                return false;
            }
            std::string lhsName(stmt->stmt.asgn.lhs->var.name);
            if (!varExists(lhsName)) {
                fprintf(stderr, "DEBUG: checkStmt ast_asgn: variable %s does not exist\n", lhsName.c_str());
                return false; // Undeclared variable
            }
            
            // Check rhs expression
            bool exprOk = checkExpr(stmt->stmt.asgn.rhs);
            if (!exprOk) {
                fprintf(stderr, "DEBUG: checkStmt ast_asgn: checkExpr failed for rhs\n");
            }
            return exprOk;
        }
            
        case ast_ret:
            if (stmt->stmt.ret.expr == NULL) return false;
            return checkExpr(stmt->stmt.ret.expr);
            
        case ast_call: {
            // Check if it's a print/read call (these are extern, always valid)
            if (stmt->stmt.call.name != NULL) {
                std::string callName(stmt->stmt.call.name);
                if (callName == "print" || callName == "read") {
                    // For print, check the parameter expression
                    if (callName == "print" && stmt->stmt.call.param != NULL) {
                        return checkExpr(stmt->stmt.call.param);
                    }
                    return true; // read() has no parameters
                }
            }
            return false; // Unknown function call
        }
            
        case ast_block:
            return checkBlock(stmt);
            
        case ast_if:
            if (stmt->stmt.ifn.cond == NULL || stmt->stmt.ifn.if_body == NULL) return false;
            
            // Check condition expression
            if (!checkExpr(stmt->stmt.ifn.cond)) return false;
            
            // Check if body
            if (!checkBlock(stmt->stmt.ifn.if_body)) return false;
            
            // Check else body if it exists
            if (stmt->stmt.ifn.else_body != NULL) {
                return checkBlock(stmt->stmt.ifn.else_body);
            }
            return true;
            
        case ast_while:
            if (stmt->stmt.whilen.cond == NULL || stmt->stmt.whilen.body == NULL) return false;
            
            // Check condition expression
            if (!checkExpr(stmt->stmt.whilen.cond)) return false;
            
            // Check while body
            return checkBlock(stmt->stmt.whilen.body);
            
        default:
            return false;
    }
}

bool checkExpr(astNode* expr)
{
    if (expr == NULL) {
        fprintf(stderr, "DEBUG: checkExpr: expr is NULL\n");
        return false;
    }
    
    switch (expr->type) {
        case ast_var: {
            std::string varName(expr->var.name);
            if (!varExists(varName)) {
                fprintf(stderr, "DEBUG: checkExpr: variable %s does not exist\n", varName.c_str());
                return false; // Undeclared variable
            }
            return true;
        }
        
        case ast_cnst:
            return true; // Constants are always valid
            
        case ast_rexpr:
            if (expr->rexpr.lhs == NULL || expr->rexpr.rhs == NULL) {
                fprintf(stderr, "DEBUG: checkExpr: ast_rexpr has NULL lhs or rhs\n");
                return false;
            }
            return checkExpr(expr->rexpr.lhs) && checkExpr(expr->rexpr.rhs);
            
        case ast_bexpr:
            if (expr->bexpr.lhs == NULL || expr->bexpr.rhs == NULL) {
                fprintf(stderr, "DEBUG: checkExpr: ast_bexpr has NULL lhs or rhs\n");
                return false;
            }
            return checkExpr(expr->bexpr.lhs) && checkExpr(expr->bexpr.rhs);
            
        case ast_uexpr:
            if (expr->uexpr.expr == NULL) {
                fprintf(stderr, "DEBUG: checkExpr: ast_uexpr has NULL expr\n");
                return false;
            }
            return checkExpr(expr->uexpr.expr);
        
        case ast_stmt:
            // Handle cases where statements (like read()) are used as expressions
            if (expr->stmt.type == ast_call) {
                std::string callName(expr->stmt.call.name);
                if (callName == "read") {
                    return true; // read() is valid as an expression
                } else if (callName == "print") {
                    // print() can be used but returns void, so it's not really an expression
                    // But for semantic analysis, we'll allow it
                    if (expr->stmt.call.param != NULL) {
                        return checkExpr(expr->stmt.call.param);
                    }
                    return true;
                }
            }
            fprintf(stderr, "DEBUG: checkExpr: ast_stmt type %d not handled as expression\n", (int)expr->stmt.type);
            return false;
            
        default:
            fprintf(stderr, "DEBUG: checkExpr: unknown expr type %d\n", (int)expr->type);
            return false;
    }
}