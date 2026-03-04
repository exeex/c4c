# tiny-c2ll

Super tiny C-subset compiler that outputs LLVM IR text without linking LLVM libraries.
Uses Python `match/case`, so run with Python 3.10+ (tested with 3.14).

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

## Progress Scripts

```bash
# Default mode: python frontend (src/frontend/c2ll.py)
bash scripts/check_progress.sh
bash scripts/full_scan.sh

# C++ frontend mode: uses ./build_debug/tiny-c2ll-stage1
COMPILER_MODE=cxx bash scripts/check_progress.sh
COMPILER_MODE=cxx bash scripts/full_scan.sh
```

### Optional: Run `c-testsuite` Allowlist

1. Clone `c-testsuite` locally (any path you choose).
2. Add supported test files into `tests/c_testsuite_allowlist.txt`.
3. Configure with testsuite root:

```bash
cmake -S . -B build -DC_TESTSUITE_ROOT=/path/to/c-testsuite
cmake --build build
ctest --test-dir build --output-on-failure -R c_testsuite_allowlist
```

`run_c_testsuite.py` enforces all selected allowlist cases must pass.
If `tests/c-testsuite` exists in this repo, CMake auto-enables it without `-DC_TESTSUITE_ROOT`.
The runner validates end-to-end behavior: `c2ll -> .ll -> clang -> executable`.
Failure logs are split by phase under `build/c_testsuite/logs/`:
- `frontend_fail.log` (`c2ll.py` compile-time failures)
- `backend_fail.log` (`clang` compile-time failures)
- `runtime_fail.log` (non-zero exit or output mismatch)

## Manual Usage

```bash
python3.14 src/frontend/c2ll.py tests/example.c -o /tmp/out.ll
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
