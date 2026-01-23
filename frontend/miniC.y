%code requires {
    #include<vector>
    #include<string>
    #include "ast/ast.h"
}

%{
    #include<stdio.h>
    #include "ast/ast.h"
    #include "symantic_analysis.h"
    extern int yylex();
    extern int yylex_destroy();
    extern int yywrap();
    int yyerror(char*);
    extern FILE *yyin;

    astNode* root;
%}

%union {
    astNode* node;
    std::vector<astNode*>* stmt_list;
    char* name;
    int num;
}

%token INT
%token RETURN
%token <num> NUMBER
%token <name> IDENTIFIER
%token EXTERN
%token VOID
%token PRINT
%token READ
%token IF
%token ELSE
%token WHILE

%type <node> prog extern_print extern_read func param block_stmt stmt decl assign return print_fn read_fn expr arithmetic_expr relational_expr term if_block while_block
%type <stmt_list> stmt_list decl_list

%start prog

%%
prog : extern_print extern_read func { root = createProg($1, $2, $3); }
extern_print : EXTERN VOID PRINT '(' INT ')' ';' { $$ = createExtern("print"); }
extern_read : EXTERN INT READ '(' ')' ';' { $$ = createExtern("read"); }
func : INT IDENTIFIER '(' param ')' block_stmt { $$ = createFunc($2, $4, $6); }
param : INT IDENTIFIER { $$ = createVar($2); }
      | /* empty */ { $$ = NULL; }
block_stmt : '{' decl_list stmt_list '}' { 
                                            $2->insert($2->end(), $3->begin(), $3->end());
                                            $$ = createBlock($2);
                                            delete $3;
                                         }
           | '{' '}' { $$ = createBlock(new std::vector<astNode*>()); }
stmt_list : stmt_list stmt { $1->push_back($2); $$ = $1; }
          | /* empty */ { $$ = new std::vector<astNode*>(); }
decl_list : decl_list decl { $1->push_back($2); $$ = $1; }
          | /* empty */ { $$ = new std::vector<astNode*>(); }
stmt : assign
     | return
     | block_stmt
     | if_block
     | while_block
     | print_fn
if_block : IF '(' relational_expr ')' block_stmt ELSE block_stmt { $$ = createIf($3, $5, $7); }
         | IF '(' relational_expr ')' block_stmt ELSE stmt {
                                                                    std::vector<astNode*>* else_block = new std::vector<astNode*>();
                                                                    else_block->push_back($7);
                                                                    astNode* else_node = createBlock(else_block);
                                                                    $$ = createIf($3, $5, else_node);
                                                                  }
         | IF '(' relational_expr ')' block_stmt { $$ = createIf($3, $5); }
         | IF '(' relational_expr ')' stmt {
                                             std::vector<astNode*>* then_block = new std::vector<astNode*>();
                                             then_block->push_back($5);
                                             astNode* then_node = createBlock(then_block);
                                             $$ = createIf($3, then_node);
                                           }
         | IF '(' relational_expr ')' stmt ELSE block_stmt {
                                                             std::vector<astNode*>* then_block = new std::vector<astNode*>();
                                                             then_block->push_back($5);
                                                             astNode* then_node = createBlock(then_block);
                                                             $$ = createIf($3, then_node, $7);
                                                           }
         | IF '(' relational_expr ')' stmt ELSE stmt {
                                                       std::vector<astNode*>* then_block = new std::vector<astNode*>();
                                                       then_block->push_back($5);
                                                       astNode* then_node = createBlock(then_block);
                                                       std::vector<astNode*>* else_block = new std::vector<astNode*>();
                                                       else_block->push_back($7);
                                                       astNode* else_node = createBlock(else_block);
                                                       $$ = createIf($3, then_node, else_node);
                                                     }
while_block : WHILE '(' relational_expr ')' block_stmt { $$ = createWhile($3, $5); }
            | WHILE '(' relational_expr ')' stmt {
                                                   std::vector<astNode*>* body_block = new std::vector<astNode*>();
                                                   body_block->push_back($5);
                                                   astNode* body_node = createBlock(body_block);
                                                   $$ = createWhile($3, body_node);
                                                 }
decl : INT IDENTIFIER ';' { $$ = createDecl($2); }
assign : IDENTIFIER '=' arithmetic_expr ';' { $$ = createAsgn(createVar($1), $3); }
       | IDENTIFIER '=' read_fn ';' { $$ = createAsgn(createVar($1), $3); }
return : RETURN expr ';' { $$ = createRet($2); }
expr : arithmetic_expr | relational_expr
print_fn : PRINT '(' arithmetic_expr ')' ';' { $$ = createCall("print", $3); }
read_fn : READ '(' ')' { $$ = createCall("read", NULL); }
arithmetic_expr : term { $$ = $1; }
                | '-' term { $$ = createUExpr($2, uminus); }
                | term '*' term { $$ = createBExpr($1, $3, mul); }
                | term '+' term { $$ = createBExpr($1, $3, add); }
                | term '-' term { $$ = createBExpr($1, $3, sub); }
relational_expr : term '<' term { $$ = createRExpr($1, $3, lt); }
                | term '<' '=' term { $$ = createRExpr($1, $4, le); }
                | term '>' term { $$ = createRExpr($1, $3, gt); }
                | term '>' '=' term { $$ = createRExpr($1, $4, ge); }
                | term '=' '=' term { $$ = createRExpr($1, $4, eq); }
term : NUMBER { $$ = createCnst($1); }
     | IDENTIFIER { $$ = createVar($1); }
     | '(' arithmetic_expr ')' { $$ = $2; }
     | '(' relational_expr ')' { $$ = $2; }
%%

int yyerror(char *s) {
    fprintf(stderr, "%s\n", s);
    return 0;
}

int main(int argc, char* argv[]) {
    if (argc == 2) {
        yyin = fopen(argv[1], "r");
        if (yyin == NULL) {
            printf("Error\n");
            return(1);
        }
    }

    yyparse();
    
    if (argc == 2) fclose(yyin);
    
    // Perform semantic analysis
    if (checkProg(root)) {
        // Semantic analysis passed
    } else {
        fprintf(stderr, "Faulty program: check for errors in the grammar\n");
        yylex_destroy();
        return(1);
    }
    
    yylex_destroy();
    return(0);
}