# tiny-c2ll Plan

## Current Progress

- Project structure has been organized:
  - `src/` compiler source
  - `tests/` local tests + c-testsuite integration
  - `docs/` spec documents
  - `CMakeLists.txt` for build/test orchestration
- Core compiler (`src/c2ll.py`) currently supports:
  - `int` / `void` function types
  - function declarations and definitions
  - `extern` function declarations
  - function parameters (`int`, `void` in parameter list)
  - function calls in expressions/statements
  - variable declaration/assignment/return
  - global `int` variables with constant initializers
  - arithmetic/logic expressions:
    - arithmetic: `+ - * / %`
    - comparison: `< > <= >= == !=`
    - bitwise/logical: `& | ^ && ||`
    - unary: `+ - !`
    - inc/dec: `++ --` (prefix/postfix)
    - compound assignment: `+= -= *= /= %=`
  - control flow:
    - `if/else`
    - `while`
    - `for`
    - `do/while`
    - `break` / `continue`
  - semantic checks:
    - redeclaration / undeclared use
    - uninitialized variable use
    - function signature conflicts
    - function call arity/type checks
  - temporary external preprocessing via `clang -E -P`
- End-to-end test pipeline is in place:
  - `c2ll.py -> .ll -> clang -> executable`
  - failure logs split into frontend/backend/runtime
- c-testsuite status:
  - Full run: `54 / 220` pass
  - Allowlist is updated to these 54 passing cases

## Next Steps (Priority Order)

1. Extend parser/lexer/sema for common C constructs causing the next largest failures:
   - array/index `[]`
   - struct member access `.`
   - pointer/address ops `& *` + pointer declarations
2. Expand expression grammar:
   - ternary `?:`
   - `sizeof`
   - remaining assignment/bitwise variants as needed
3. Add minimal control flow:
   - `switch/case/default`
   - `goto` / labels
4. Keep growing `tests/c_testsuite_allowlist.txt` incrementally after each feature.

## How To Run Tests

### Configure + build

```bash
cmake -S . -B build
cmake --build build
```

### Run all configured tests

```bash
ctest --test-dir build --output-on-failure
```

### Run only local regression tests

```bash
python3.14 tests/run_tests.py \
  --compiler src/c2ll.py \
  --workdir /tmp/tiny-c2ll-test \
  --clang /usr/bin/clang
```

### Run c-testsuite allowlist (end-to-end)

```bash
python3.14 tests/run_c_testsuite.py \
  --compiler src/c2ll.py \
  --clang /usr/bin/clang \
  --testsuite-root tests/c-testsuite \
  --workdir build \
  --allowlist tests/c_testsuite_allowlist.txt
```

### Run full c-testsuite scan (all cases)

```bash
python3.14 tests/run_c_testsuite.py \
  --compiler src/c2ll.py \
  --clang /usr/bin/clang \
  --testsuite-root tests/c-testsuite \
  --workdir build
```

### Failure logs location

- `build/c_testsuite/logs/frontend_fail.log`
- `build/c_testsuite/logs/backend_fail.log`
- `build/c_testsuite/logs/runtime_fail.log`
