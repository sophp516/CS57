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

    // Check if variable already exists in current scope
    std::vector<std::string>& currScope = symbolTable.back();
    for (size_t i = 0; i < currScope.size(); i++) {
        if (currScope[i] == varName) {
            return false;
        }
    }
    
    // Check if variable exists in any parent scope (can't redeclare/shadow)
    if (varExists(varName)) {
        return false;
    }
    
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
    if (root == NULL || root->type != ast_prog) return false;
    
    return checkFunc(root->prog.func);
}

bool checkFunc(astNode* func)
{
    if (func == NULL || func->type != ast_func) return false;
    
    pushScope();
    
    if (func->func.param != NULL) {
        if (func->func.param->type == ast_var) {
            std::string paramName(func->func.param->var.name);
            if (!addVarToScope(paramName)) {
                popScope();
                return false;
            }
        }
    }
    
    bool result = checkBlock(func->func.body);
    
    popScope();
    
    return result;
}

bool checkBlock(astNode* block)
{
    if (block == NULL || block->type != ast_stmt || block->stmt.type != ast_block) {
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
            
        case ast_asgn:
            // Check lhs (variable being assigned to) and rhs (expression)
            if (stmt->stmt.asgn.lhs == NULL || stmt->stmt.asgn.rhs == NULL) return false;
            
            // Check lhs is a variable
            if (stmt->stmt.asgn.lhs->type != ast_var) return false;
            std::string lhsName(stmt->stmt.asgn.lhs->var.name);
            if (!varExists(lhsName)) {
                return false; // Undeclared variable
            }
            
            // Check rhs expression
            return checkExpr(stmt->stmt.asgn.rhs);
            
        case ast_ret:
            if (stmt->stmt.ret.expr == NULL) return false;
            return checkExpr(stmt->stmt.ret.expr);
            
        case ast_call:
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
    if (expr == NULL) return false;
    
    switch (expr->type) {
        case ast_var: {
            std::string varName(expr->var.name);
            if (!varExists(varName)) {
                return false; // Undeclared variable
            }
            return true;
        }
        
        case ast_cnst:
            return true; // Constants are always valid
            
        case ast_rexpr:
            if (expr->rexpr.lhs == NULL || expr->rexpr.rhs == NULL) return false;
            return checkExpr(expr->rexpr.lhs) && checkExpr(expr->rexpr.rhs);
            
        case ast_bexpr:
            if (expr->bexpr.lhs == NULL || expr->bexpr.rhs == NULL) return false;
            return checkExpr(expr->bexpr.lhs) && checkExpr(expr->bexpr.rhs);
            
        case ast_uexpr:
            if (expr->uexpr.expr == NULL) return false;
            return checkExpr(expr->uexpr.expr);
            
        default:
            return false;
    }
}