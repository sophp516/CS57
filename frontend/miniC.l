%{
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include "y.tab.h"
int line = 1;
%}

%%
\n          { line++; }
extern      { printf("Line %d: EXTERN\n", line); return EXTERN; }
int         { printf("Line %d: INT\n", line); return INT; }
void        { printf("Line %d: VOID\n", line); return VOID; }
return      { printf("Line %d: RETURN\n", line); return RETURN; }
print       { printf("Line %d: PRINT\n", line); return PRINT; }
read        { printf("Line %d: READ\n", line); return READ; }
if          { printf("Line %d: IF\n", line); return IF; }
else        { printf("Line %d: ELSE\n", line); return ELSE; }
while       { printf("Line %d: WHILE\n", line); return WHILE; }

"{"         { printf("Line %d: '{'\n", line); return '{'; }
"}"         { printf("Line %d: '}'\n", line); return '}'; }
"("         { printf("Line %d: '('\n", line); return '('; }
")"         { printf("Line %d: ')'\n", line); return ')'; }
";"         { printf("Line %d: ';'\n", line); return ';'; }

"="         { printf("Line %d: '='\n", line); return '='; }
">"         { printf("Line %d: '>'\n", line); return '>'; }
"<"         { printf("Line %d: '<'\n", line); return '<'; }
"*"         { printf("Line %d: '*'\n", line); return '*'; }
"+"         { printf("Line %d: '+'\n", line); return '+'; }
"-"         { printf("Line %d: '-'\n", line); return '-'; }

[0-9]+      { 
                yylval.num = atoi(yytext);
                printf("Line %d: NUMBER (%d)\n", line, yylval.num);
                return NUMBER;
            }

[a-zA-Z][a-zA-Z0-9]*    {   
                            yylval.name = strdup(yytext);
                            printf("Line %d: IDENTIFIER (%s)\n", line, yytext);
                            return IDENTIFIER;
                        }

[ \t]+      { /* skip whitespace */ }

.           { printf("Line %d: UNKNOWN CHAR '%c'\n", line, yytext[0]); return yytext[0]; }
%%

int yywrap() {
    return(1);
}

