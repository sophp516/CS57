#ifndef SEMANTIC_ANALYSIS_H
#define SEMANTIC_ANALYSIS_H

#include "frontend/ast/ast.h"

// Semantic analysis functions
bool checkProg(astNode* root);
bool checkFunc(astNode* func);
bool checkBlock(astNode* block);
bool checkStmt(astNode* stmt);
bool checkExpr(astNode* expr);

#endif // SEMANTIC_ANALYSIS_H
