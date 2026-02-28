#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "frontend/ast/ast.h"
#include "frontend/y.tab.h"
#include "semantic_analysis.h"
#include "ir_builder.h"
#include "optimizer.h"
#include <llvm-c/Core.h>
#include <llvm-c/IRReader.h>

// External variables from parser
extern astNode* root;
extern FILE *yyin;
extern int yyparse();
extern int yylex_destroy();
extern int yywrap();

int main(int argc, char** argv) {

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <input_file>\n", argv[0]);
        return 1;
    }
    
    char* filename = argv[1];
    
    // open input file
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        fprintf(stderr, "Error: Cannot open file '%s'\n", filename);
        return 1;
    }
    
    yyin = file;
    
    // initialize root to NULL
    root = NULL;
    
    // run syntax analysis and build AST
    int parse_result = yyparse();
    
    fclose(file);
    
    if (parse_result != 0 || root == NULL) {
        fprintf(stderr, "Error: Parsing failed\n");
        yylex_destroy();
        return 1;
    }
    
    // run semantic analysis
    bool semantic_ok = checkProg(root);
    if (!semantic_ok) {
        fprintf(stderr, "Error: Semantic analysis failed\n");
        freeNode(root);
        yylex_destroy();
        return 1;
    }
    
    // run IR builder to get LLVM IR module
    LLVMModuleRef module = buildIR(root);
    if (module == NULL) {
        fprintf(stderr, "Error: IR generation failed\n");
        freeNode(root);
        yylex_destroy();
        return 1;
    }
    
    // run optimizer
    LLVMModuleRef optimized_module = optimizeModule(module);
    if (optimized_module == NULL) {
        fprintf(stderr, "Warning: Optimization failed, using unoptimized module\n");
        optimized_module = module;
    }
    
    // Print optimized module to file for verification
    char output_file[256];
    snprintf(output_file, sizeof(output_file), "%s.ll", filename);
    LLVMPrintModuleToFile(optimized_module, output_file, NULL);
    
    freeNode(root);
    yylex_destroy();
    LLVMDisposeModule(optimized_module);
    LLVMShutdown();
    
    return 0;
}
