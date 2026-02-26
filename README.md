# tiny-c2ll

Super tiny C-subset compiler that outputs LLVM IR text without linking LLVM libraries.
Uses Python `match/case`, so run with Python 3.10+ (tested with 3.14).

## Project Layout

- `src/c2ll.py`: compiler entry point and implementation
- `tests/`: test inputs and test runner
- `docs/SPEC.md`: language and roadmap spec
- `CMakeLists.txt`: build/test orchestration

## Supported Syntax

- One function: `int <name>() { ... }`
- Statements:
  - `int x;`
  - `int x = expr;`
  - `x = expr;`
  - `return expr;`
- Expressions:
  - integer literal, variable
  - `+ - * /`
  - parentheses

## Build With CMake

```bash
cmake -S . -B build
cmake --build build
```

This generates `build/example.ll` through the `build_example_ir` target.

Optional executable build (requires `clang`):

```bash
cmake --build build --target build_example_bin
```

## Run Tests

```bash
ctest --test-dir build --output-on-failure
```

## Manual Usage

```bash
python3.14 src/c2ll.py tests/example.c -o /tmp/out.ll
clang /tmp/out.ll -o /tmp/out
/tmp/out
echo $?
```

`echo $?` prints the C `main` return value.

## Pipeline

- Lexer: source text -> token stream
- Parser: token stream -> AST
- SemanticAnalyzer: validates declarations, assignments, and uninitialized reads
- IRBuilder: AST -> LLVM IR text

## Next

- Language roadmap and pattern-matching design: `docs/SPEC.md`
