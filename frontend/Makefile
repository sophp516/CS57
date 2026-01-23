CC=g++
LEX=lex
YACC=yacc
CFLAGS=-Wall -std=c++11
LDFLAGS=-ll -ly

TARGET=miniC.out
LEX_SOURCE=miniC.l
YACC_SOURCE=miniC.y
AST_SOURCE=ast/ast.c
SEMANTIC_SOURCE=semantic_analysis.c
LEX_GENERATED=lex.yy.c
YACC_GENERATED=y.tab.c y.tab.h

$(TARGET): $(LEX_GENERATED) $(YACC_GENERATED) $(AST_SOURCE) $(SEMANTIC_SOURCE)
	$(CC) $(CFLAGS) $(LEX_GENERATED) y.tab.c $(AST_SOURCE) $(SEMANTIC_SOURCE) -o $(TARGET) $(LDFLAGS)

$(LEX_GENERATED): $(LEX_SOURCE) y.tab.h
	$(LEX) $(LEX_SOURCE)

y.tab.c y.tab.h: $(YACC_SOURCE)
	$(YACC) -d $(YACC_SOURCE)

clean:
	rm -f $(LEX_GENERATED) $(YACC_GENERATED) $(TARGET) *.o

.PHONY: clean

