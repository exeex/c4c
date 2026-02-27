# tiny-c2ll Plan

## Current Progress

- Project structure has been organized:
  - `src/` compiler source
  - `tests/` local tests + c-testsuite integration
  - `docs/` spec documents
  - `CMakeLists.txt` for build/test orchestration
- Core compiler (`src/c2ll.py`) currently supports:
  - `int` / `void` / `char` / `short` / `long` / `unsigned` / `double` function and variable types
  - function declarations and definitions
  - `extern` function declarations
  - function parameters (`int`, `void` in parameter list)
  - function calls in expressions/statements
  - indirect calls through function pointers
  - variable declaration/assignment/return
  - local `static` variables
  - global `int` variables with constant initializers
  - basic pointer/addressing:
    - pointer declarations (`int *p`)
    - address-of/dereference (`&` / `*`) for scalar variables
    - array/pointer indexing (`a[i]`, `p[i]`)
    - multi-level pointers (`int **pp`, `**pp`)
    - pointer arithmetic (`p +/- n`, `p++/--`, `p += n`, pointer difference)
  - basic struct support:
    - `struct` type declarations (named + anonymous)
    - `union` parsing (including anonymous union members in structs)
    - member access via `.` and `->`
    - struct fields that are arrays
  - basic typedef + cast support:
    - type aliases (`typedef ...`)
    - simple C-style casts `(T)expr`
    - compound literals `(T){...}`
  - arithmetic/logic expressions:
    - arithmetic: `+ - * / %`
    - comparison: `< > <= >= == !=`
    - bitwise/logical: `& | ^ && ||`
    - unary: `+ - !`
    - `sizeof` (expression + type form)
    - ternary `?:`
    - floating literals and double arithmetic/comparison
    - inc/dec: `++ --` (prefix/postfix)
    - compound assignment: `+= -= *= /= %=`
  - control flow:
    - `if/else`
    - `while`
    - `for`
    - `do/while`
    - `break` / `continue`
    - labels / `goto` (minimal)
    - `switch/case/default` (minimal)
    - statement expressions `({ ... })`
  - initializer coverage:
    - integer array initializers (including designated index form like `[i] = v`)
    - struct initializers (positional + designated field)
    - array-of-struct initializers
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
  - Allowlist run (`2026-02-27`): `188 / 188` pass, `0` fail
  - Overall milestone: `188 / 220` passing coverage represented in allowlist
  - `tests/c_testsuite_allowlist.txt` has been updated to these 188 passing cases

## Next Steps (Priority Order)

1. Close the remaining `32` c-testsuite cases (focus on the top repeated failure signatures in frontend/backend/runtime logs).
2. Improve aggregate initializer completeness for deeper nested combinations and edge cases.
3. Continue growing `tests/c_testsuite_allowlist.txt` and keep `plan.md` in sync per milestone.

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
