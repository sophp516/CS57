#include<stdio.h>
#include<vector>
#include<string>
#include "ast/ast.h"

std::vector<std::vector<std::string>> symbolTable;

// helper functions
bool varExists(std::string varName); 

bool addVarToScope(std::string varName);

bool pushScope();

bool popScope();


// functions for symantic analysis
bool checkProg(astNode* root);

bool checkFunc(astNode* func);

bool checkBlock(astNode* block);

bool checkStmt(astNode* stmt);

bool checkExpr(astNode* expr);
