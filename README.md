# c4c

`c4c` is a lightweight C-family frontend that lowers source code to LLVM IR.

Today the project is best understood as:

- a custom frontend pipeline implemented in C++
- an LLVM IR text emitter
- a test-heavy migration project that still relies on `clang` as the executable backend

The frontend primarily targets C input, but the current codebase and test suite also
contain experimental C++-style features such as templates and `consteval`.

## Architecture

Current pipeline:

1. Preprocessor: `.c` / `.h` / `.cpp` -> preprocessed source text
2. Lexer: source text -> token stream
3. Parser: token stream -> AST
4. Semantic analysis: AST validation + canonical typing + HIR lowering
5. HIR passes: compile-time reduction + materialization / inline expansion
6. LLVM IR builder: HIR -> `.ll`
7. Backend: `clang` consumes emitted `.ll` and produces a native binary

The main executable is `c4cll`.

## Repository Layout

```txt
src/
  apps/                CLI entrypoints
  frontend/
    preprocessor/      macro expansion, includes, conditionals, pragmas
    lexer/             tokenization
    parser/            recursive-descent parser and AST
    sema/              validation, canonical symbols, consteval/type logic
    hir/               AST -> HIR lowering and HIR passes
  codegen/llvm/        HIR -> LLVM IR emission

tests/
  internal/            project-owned positive / negative / runtime cases
  preprocessor/        focused preprocessor unit-style tests
  external/            vendored or derived compatibility suites

scripts/               local workflows and automation helpers
ref/                   reference implementations and upstream corpora
```

## Build

```bash
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j
```

Notes:

- `clang` is optional for building `c4cll` itself.
- `clang` is required for runtime / execute-style tests and for turning emitted LLVM IR
  into a runnable binary.

## Quick Start

Emit LLVM IR:

```bash
./build/c4cll tests/internal/example/example.c -o hello.ll
```

Build and run the emitted program with `clang`:

```bash
clang hello.ll -o hello.out
./hello.out
```

Pipe directly:

```bash
./build/c4cll tests/internal/example/example.c | clang -x ir -o hello.out -
./hello.out
```

## Useful CLI Modes

`c4cll` exposes several frontend-debugging modes:

- `--pp-only` print preprocessed source
- `--lex-only` dump tokens
- `--parse-only` dump AST
- `--dump-canonical` dump canonical symbol/type information
- `--dump-hir` dump full HIR plus compile-time/materialization stats
- `--dump-hir-summary` dump a compact HIR summary

Example:

```bash
./build/c4cll --dump-hir tests/internal/example/example.c
```

## Testing

CTest is the main test entrypoint.

Core test layers in this repository:

- `tests/internal/positive_case`: frontend + IR + runtime positive coverage
- `tests/internal/negative_case`: expected compile failures
- `tests/preprocessor`: focused preprocessor tests
- `tests/external/c-testsuite`: curated external C coverage
- `tests/external/gcc_torture`: curated tricky / migration-heavy cases

Run the default core suite:

```bash
cd build
ctest --output-on-failure
```

Or use the convenience target:

```bash
cd build
cmake --build . --target ctest_core
```

## Current Status

- Frontend stages are implemented in-repo.
- LLVM IR emission is implemented in-repo.
- Native code generation is delegated to external `clang`.
- The project is actively driven by regression tests and incremental bug fixing.

## Development Workflow

Typical loop:

1. Add or adjust a case under `tests/`
2. Rebuild `c4cll`
3. Run the relevant CTest subset
4. Inspect `--pp-only`, `--parse-only`, or `--dump-hir` output when debugging

There is also a local automation helper:

```bash
./scripts/run_agent.sh
```

Use it only if your environment is set up for that workflow.
