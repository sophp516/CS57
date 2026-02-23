# CS57 MiniC Compiler

A complete compiler for the MiniC language that performs lexical analysis, parsing, semantic analysis, IR generation, and optimization.

## Project Structure

```
assignments/
├── frontend/              # Lexer, parser, and semantic analysis
│   ├── miniC.l            # Lex specification
│   ├── miniC.y            # Yacc grammar specification
│   ├── semantic_analysis.c # Semantic analysis implementation
│   └── ast/               # AST data structures
├── llvm_ir_builder/       # LLVM IR generation (pseudocode)
│   └── builder.c          # IR builder pseudocode
├── optimizations/         # LLVM IR optimizations
│   └── llvm_parser.c      # Optimization passes
├── semantic_analysis.h    # Semantic analysis header
├── ir_builder.h           # IR builder header
├── ir_builder.c           # IR builder wrapper (to be implemented)
├── optimizer.h            # Optimizer header
├── optimizer.c            # Optimizer wrapper
├── main.c                 # Main entry point
├── Makefile               # Build configuration
└── README.md              # This file
```

## Prerequisites

- GCC/G++ compiler
- Flex (lexer generator)
- Bison (parser generator)
- LLVM 17 development libraries
- Make

## Building the Compiler

To build the compiler, simply run:

```bash
make
```

This will:
1. Generate the lexer from `frontend/miniC.l`
2. Generate the parser from `frontend/miniC.y`
3. Compile all source files
4. Link everything into the executable `miniC_compiler`

## Running the Compiler

To compile a MiniC program:

```bash
./miniC_compiler <input_file>
```

Where `<input_file>` is a file containing MiniC source code.

### Example

```bash
./miniC_compiler frontend/parser_tests/p1.c
```

## Compiler Pipeline

The compiler performs the following steps:

1. **Lexical Analysis**: Tokenizes the input using Flex
2. **Syntax Analysis**: Parses tokens into an AST using Bison
3. **Semantic Analysis**: Validates the AST (variable declarations, scope checking)
4. **IR Generation**: Converts AST to LLVM IR
5. **Optimization**: Applies optimization passes to the LLVM IR
6. **Cleanup**: Frees all allocated resources

## Error Handling

The compiler will exit with an error code if:
- The input file cannot be opened
- Syntax errors are detected during parsing
- Semantic errors are detected (undeclared variables, etc.)
- IR generation fails

## Cleaning Build Artifacts

To remove all generated files and object files:

```bash
make clean
```

## Notes

- The parser automatically generates `y.tab.c` and `y.tab.h` from `miniC.y`
- The lexer automatically generates `lex.yy.c` from `miniC.l`
- All generated files are placed in their respective directories
- The final executable is named `miniC_compiler`
