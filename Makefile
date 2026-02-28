# Compiler and tools
CC = gcc
CXX = g++
LEX = flex
YACC = bison
LLVM_CONFIG = llvm-config-17

# Directories
FRONTEND_DIR = frontend
IR_BUILDER_DIR = llvm_ir_builder
OPTIMIZER_DIR = optimizations
AST_DIR = $(FRONTEND_DIR)/ast

# Compiler flags
CFLAGS = -Wall -g -I /usr/include/llvm-c-17/
CXXFLAGS = -Wall -g -std=c++11 -I /usr/include/llvm-c-17/
LDFLAGS = -ll -ly

# LLVM flags
LLVM_CFLAGS = $(shell $(LLVM_CONFIG) --cxxflags)
LLVM_LDFLAGS = $(shell $(LLVM_CONFIG) --ldflags --libs core)

# Source files
LEX_SOURCE = $(FRONTEND_DIR)/miniC.l
YACC_SOURCE = $(FRONTEND_DIR)/miniC.y
AST_SOURCE = $(AST_DIR)/ast.c
SEMANTIC_SOURCE = $(FRONTEND_DIR)/semantic_analysis.c
IR_BUILDER_SOURCE = ir_builder.c $(IR_BUILDER_DIR)/builder.c
OPTIMIZER_SOURCE = $(OPTIMIZER_DIR)/llvm_parser.c optimizer.c
MAIN_SOURCE = main.c

# Generated files
LEX_GENERATED = $(FRONTEND_DIR)/lex.yy.c
YACC_GENERATED = $(FRONTEND_DIR)/y.tab.c $(FRONTEND_DIR)/y.tab.h

# Object files
AST_OBJ = $(AST_DIR)/ast.o
SEMANTIC_OBJ = $(FRONTEND_DIR)/semantic_analysis.o
IR_BUILDER_OBJ = ir_builder.o
OPTIMIZER_OBJ = $(OPTIMIZER_DIR)/llvm_parser.o optimizer.o
MAIN_OBJ = main.o
LEX_OBJ = $(FRONTEND_DIR)/lex.yy.o
YACC_OBJ = $(FRONTEND_DIR)/y.tab.o

# Target executable
TARGET = miniC_compiler

# Default target
all: $(TARGET)

# Main executable
$(TARGET): $(LEX_OBJ) $(YACC_OBJ) $(AST_OBJ) $(SEMANTIC_OBJ) ir_builder.o $(IR_BUILDER_DIR)/builder.o $(OPTIMIZER_DIR)/llvm_parser.o optimizer.o $(MAIN_OBJ)
	$(CXX) $(CXXFLAGS) $(LEX_OBJ) $(YACC_OBJ) $(AST_OBJ) $(SEMANTIC_OBJ) ir_builder.o $(IR_BUILDER_DIR)/builder.o $(OPTIMIZER_DIR)/llvm_parser.o optimizer.o $(MAIN_OBJ) -o $(TARGET) $(LDFLAGS) $(LLVM_LDFLAGS)

# Generate lexer
$(LEX_GENERATED): $(LEX_SOURCE) $(FRONTEND_DIR)/y.tab.h
	$(LEX) -o $(LEX_GENERATED) $(LEX_SOURCE)

# Generate parser
$(FRONTEND_DIR)/y.tab.c $(FRONTEND_DIR)/y.tab.h: $(YACC_SOURCE)
	$(YACC) -d -o $(FRONTEND_DIR)/y.tab.c $(YACC_SOURCE)

# Compile object files
$(AST_OBJ): $(AST_SOURCE) $(AST_DIR)/ast.h
	$(CXX) $(CXXFLAGS) -c $(AST_SOURCE) -o $(AST_OBJ)

$(SEMANTIC_OBJ): $(SEMANTIC_SOURCE) semantic_analysis.h
	$(CXX) $(CXXFLAGS) -c $(SEMANTIC_SOURCE) -o $(SEMANTIC_OBJ)

$(LEX_OBJ): $(LEX_GENERATED) $(FRONTEND_DIR)/y.tab.h
	$(CXX) $(CXXFLAGS) -c $(LEX_GENERATED) -o $(LEX_OBJ)

$(YACC_OBJ): $(FRONTEND_DIR)/y.tab.c
	$(CXX) $(CXXFLAGS) -c $(FRONTEND_DIR)/y.tab.c -o $(YACC_OBJ)

ir_builder.o: ir_builder.c ir_builder.h
	$(CXX) $(CXXFLAGS) -c ir_builder.c -o ir_builder.o

$(IR_BUILDER_DIR)/builder.o: $(IR_BUILDER_DIR)/builder.c ir_builder.h
	$(CXX) $(CXXFLAGS) -c $(IR_BUILDER_DIR)/builder.c -o $(IR_BUILDER_DIR)/builder.o

$(OPTIMIZER_DIR)/llvm_parser.o: $(OPTIMIZER_DIR)/llvm_parser.c optimizer.h
	$(CXX) $(CXXFLAGS) -c $(OPTIMIZER_DIR)/llvm_parser.c -o $(OPTIMIZER_DIR)/llvm_parser.o

optimizer.o: optimizer.c optimizer.h
	$(CXX) $(CXXFLAGS) -c optimizer.c -o optimizer.o

$(MAIN_OBJ): $(MAIN_SOURCE) semantic_analysis.h ir_builder.h optimizer.h
	$(CXX) $(CXXFLAGS) -c $(MAIN_SOURCE) -o $(MAIN_OBJ)

# Clean target
clean:
	rm -f $(TARGET)
	rm -f $(LEX_GENERATED) $(YACC_GENERATED)
	rm -f $(AST_OBJ) $(SEMANTIC_OBJ) ir_builder.o $(IR_BUILDER_DIR)/builder.o $(OPTIMIZER_DIR)/llvm_parser.o optimizer.o $(MAIN_OBJ) $(LEX_OBJ) $(YACC_OBJ)
	rm -f *.o $(FRONTEND_DIR)/*.o $(IR_BUILDER_DIR)/*.o $(OPTIMIZER_DIR)/*.o $(AST_DIR)/*.o

.PHONY: all clean
