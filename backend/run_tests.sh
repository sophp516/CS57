#!/bin/bash
# Generate .ll with clang, run backend to get .s, compile with main.c, run.
# Usage: ./run_tests.sh [fact|fib|square|rem_2|max_n|sum_n]

set -e
cd "$(dirname "$0")"
BACKEND=./backend_driver
TESTS_DIR=assembly_gen_tests
MAIN=$TESTS_DIR/main.c

run_one() {
    local name=$1
    echo "=== $name ==="
    clang -S -emit-llvm -m32 $TESTS_DIR/${name}.c -o $TESTS_DIR/${name}.ll 2>/dev/null || true
    $BACKEND $TESTS_DIR/${name}.ll $TESTS_DIR/${name}.s
    clang -m32 $TESTS_DIR/${name}.s $MAIN -o $TESTS_DIR/a.out 2>/dev/null
    ./$TESTS_DIR/a.out
    echo ""
}

if [ $# -ge 1 ]; then
    for t in "$@"; do run_one "$t"; done
else
    for t in fact square; do run_one "$t"; done
fi
