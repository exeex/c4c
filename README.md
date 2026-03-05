# tiny-c2ll

Super tiny C-subset compiler that outputs LLVM IR text without linking LLVM libraries.

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

For parallel test execution:

```bash
ctest --test-dir build --output-on-failure -j 8
```

## Progress Scripts

```bash
bash scripts/check_progress.sh
bash scripts/full_scan.sh
```

Both scripts use CMake + CTest and run `c_testsuite` tests in parallel.

### Optional: Run `c-testsuite` Allowlist

1. Clone `c-testsuite` locally (any path you choose).
2. Add supported test files into `tests/c_testsuite_allowlist.txt`.
3. Configure with testsuite root:

```bash
cmake -S . -B build -DC_TESTSUITE_ROOT=/path/to/c-testsuite
cmake --build build
ctest --test-dir build --output-on-failure -L c_testsuite -j 8
```

If `tests/c-testsuite` exists in this repo, CMake auto-enables it without `-DC_TESTSUITE_ROOT`.

### Optional: Run `llvm-test-suite` `gcc-c-torture` Allowlist

1. Clone `llvm-test-suite` locally (any path you choose), or initialize this repo submodule.
2. Add supported test files into `tests/llvm_gcc_c_torture_allowlist.txt`.
3. Configure with testsuite root:

```bash
cmake -S . -B build -DLLVM_TEST_SUITE_ROOT=/path/to/llvm-test-suite
cmake --build build
ctest --test-dir build --output-on-failure -L llvm_gcc_c_torture -j 8
```

If `tests/llvm-test-suite` exists in this repo, CMake auto-enables it without `-DLLVM_TEST_SUITE_ROOT`.

## Manual Usage

```bash
./build/tiny-c2ll-stage1 tests/tiny_c2ll/example.c -o /tmp/out.ll
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

## Run agent

```bash
caffeinate -dimsu ./scripts/run_agent.sh
```
