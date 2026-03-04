# tiny-c2ll Plan

## Current Progress

- Project structure has been organized:
  - `src/` compiler source
  - `tests/` local tests + c-testsuite integration
  - `docs/` spec documents
  - `CMakeLists.txt` for build/test orchestration
- Core Python compiler (`src/frontend/c2ll.py`) currently supports:
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
    - multi-level pointers (`int **p｀p`, `**pp`)
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
  - `src/frontend_c/preprocessor.*` module is now wired in C++ stage1 driver.
    - backend priority: `ccc -E -P` -> `cc -E -P` -> `cpp -E -P` -> raw file fallback.
    - this removes hardcoded dependency on `clang -E` in `driver.cpp`.
- End-to-end test pipeline is in place:
  - `c2ll.py -> .ll -> clang -> executable`
  - failure logs split into frontend/backend/runtime
- c-testsuite status:
  - Allowlist run (`2026-03-04`): `220 / 220` pass, `0` fail
  - `tests/c_testsuite_allowlist.txt` currently tracks 220 selected cases
- Local regression status (`2026-03-04`):
  - `tiny_c2ll_tests`: pass
  - `c_testsuite_allowlist`: pass (`220 / 220`)
- C++ frontend stage1 (M0) is now in place under `src/frontend_c/`:
  - `frontend_cxx_stage1` target builds successfully.
  - CLI skeleton is wired:
    - `--version`
    - `--lex-only <file>`
    - `--parse-only <file>`
    - default mode emits LLVM IR stub
  - CTest smoke suite for C++ stage1 passes:
    - `frontend_cxx_stage1_version`
    - `frontend_cxx_stage1_lex`
    - `frontend_cxx_stage1_parse`
    - `frontend_cxx_stage1_emit_ir`

## Next Steps (Priority Order)

1. M1 lexer parity (Rust-mirrored) for `src/frontend_c/lexer.*`:
   - align token kinds and punctuation scanning with `ref/claudes-c-compiler/src/frontend/lexer/token.rs` and `scan.rs`.
   - add baseline handling for multi-char operators and keyword classification.
2. Add Python-vs-C++ lexer differential checks on focused fixtures:
   - same preprocessed input, compare token stream shape and key lexemes.
3. Extend C++ parser from summary stub to minimal AST slice used by bootstrap path.
4. Keep Python oracle green while advancing C++:
   - run `tiny_c2ll_tests` + c-testsuite allowlist before/after each milestone slice.

## Lexer Parity Follow-ups (`scan.rs` vs `src/lexer.py`)

- Add GCC line-marker skipping (`# <num> "file"`) at start-of-line in lexer (currently not handled in `src/lexer.py`).
- Improve numeric literal coverage:
  - binary literals `0b...`
  - octal semantics for leading-`0` integers
  - hex-float literals (`0x1.fp+2`)
  - imaginary suffixes (`i/j`) if we want GNU parity.
- Fix `.NN` float path in `src/lexer.py` (currently tokenizes as `NUM("0")`).
- Add escape coverage parity for `\uNNNN`, `\UNNNNNNNN`, and GNU `\e`.
- Evaluate multi-char character constants (`'ab'`) behavior; `scan.rs` folds bytes into an int, Python lexer currently treats char literals as single-char style.
- Evaluate whether to support `$` in identifiers (GNU mode behavior in `scan.rs`).
- Evaluate preprocessor synthetic tokens support (`#`, `##`, pragma-pack/visibility markers) if we expand beyond current preprocessed input assumptions.

## `ccc-review-tests` Handoff Notes

- New test suite location: `tests/ccc-review-tests/`
- CTest entry: `ccc_review_tests`
- Runner: `tests/run_ccc_review_tests.py`
- Important: this suite must run with our Python C compiler via:
  - `--compiler src/frontend/c2ll.py`
  - (runner flow is: Clang compile/run baseline first, then `python src/frontend/c2ll.py` -> `.ll` -> Clang -> run)

Current test inventory (2026-02-28):
- Expected-fail gap trackers (`// CCC_EXPECT: fail`):
  - `0001_binary_literal.c`
  - `0002_hex_float_literal.c`
  - `0003_dot_float_literal.c`
  - `0004_unicode_escape_string.c`
  - `0005_unicode_escape_char.c`
  - `0006_dollar_identifier.c`
- Expected-pass sanity checks (`// CCC_EXPECT: pass`):
  - `0007_line_marker.c`
  - `0008_octal_literal.c`

Follow-up rule for next agent:
- When a lexer gap is fixed, flip corresponding case from `CCC_EXPECT: fail` to `CCC_EXPECT: pass` and keep `ccc_review_tests` green in CTest.

## How To Run Tests

### Configure + build

```bash
cmake -S . -B build_debug
cmake --build build_debug
```

### Run all configured tests

```bash
ctest --test-dir build_debug --output-on-failure
```

### Run only ccc review tests

```bash
ctest --test-dir build_debug -R ccc_review_tests --output-on-failure
```

### Run only C++ stage1 smoke tests

```bash
ctest --test-dir build_debug -R frontend_cxx_stage1 --output-on-failure
```

### Run C++ stage1 directly

```bash
./build_debug/tiny-c2ll-stage1 --version
./build_debug/tiny-c2ll-stage1 --lex-only tests/example.c
./build_debug/tiny-c2ll-stage1 --parse-only tests/example.c
```

### Run only local regression tests

```bash
python3.14 tests/run_tests.py \
  --compiler src/frontend/c2ll.py \
  --workdir /tmp/tiny-c2ll-test \
  --clang /usr/bin/clang
```

### Run c-testsuite allowlist (end-to-end)

```bash
python3.14 tests/run_c_testsuite.py \
  --compiler src/frontend/c2ll.py \
  --clang /usr/bin/clang \
  --testsuite-root tests/c-testsuite \
  --workdir build_debug \
  --allowlist tests/c_testsuite_allowlist.txt
```

### Run full c-testsuite scan (all cases)

```bash
python3.14 tests/run_c_testsuite.py \
  --compiler src/frontend/c2ll.py \
  --clang /usr/bin/clang \
  --testsuite-root tests/c-testsuite \
  --workdir build_debug
```

### Run c-testsuite with C++ frontend binary (when milestone supports full compile)

```bash
python3.14 tests/run_c_testsuite.py \
  --compiler ./build_debug/tiny-c2ll-stage1 \
  --clang /usr/bin/clang \
  --testsuite-root tests/c-testsuite \
  --workdir build_debug
```

### Run progress scripts (python / cxx mode)

```bash
# Default: python compiler
bash scripts/check_progress.sh
bash scripts/full_scan.sh

# C++ frontend binary mode
COMPILER_MODE=cxx bash scripts/check_progress.sh
COMPILER_MODE=cxx bash scripts/full_scan.sh
```

### Failure logs location

- `build_debug/c_testsuite/logs/frontend_fail.log`
- `build_debug/c_testsuite/logs/backend_fail.log`
- `build_debug/c_testsuite/logs/runtime_fail.log`

## C++ Frontend First Plan (Rust Parity, Then Pure-C Backport)

### Target

- Implement `src/frontend_c/` in C++ first to accelerate delivery and reduce friction.
- Mirror `ref/claudes-c-compiler/src/frontend` Rust architecture (`lexer -> parser -> sema -> IR`) as the primary reference.
- Keep output as LLVM IR text (debug-friendly).
- Preserve a clear backport path to pure C after behavior parity is reached.

### Reference Mapping

- `ref/claudes-c-compiler/src/frontend`: primary structure and phase-contract reference.
- `/tmp/ref/amacc`: minimal compiler tactics, symbol-table style, pragmatic parsing ideas.
- `src/frontend/*.py`: stage0 oracle for behavior parity and regression checking.

### Proposed Source Layout

- `src/frontend_c/`
  - `common.hpp` `diag.cpp`
  - `token.hpp` `lexer.cpp`
  - `ast.hpp` `arena.cpp`
  - `parser.cpp`
  - `sema.cpp`
  - `ir_builder.cpp` (LLVM IR text emitter)
  - `driver.cpp` (`main`, `-o`, preprocess call)
  - optional C-compat shim headers for future pure-C backport checkpoints

### Bootstrap Strategy (C++ First)

1. Stage0
   - Python frontend remains correctness oracle.
2. Stage1
   - Build C++ frontend binary with `clang++`.
3. Stage2
   - C++ frontend passes smoke tests and allowlist parity targets.
4. Stage3 (planned)
   - Backport C++ modules to pure C incrementally with parity locks.

### Milestones

1. M0: Build skeleton + CMake targets
   - Add `frontend_cxx_stage1` executable target.
   - Keep existing Python compiler/test path untouched.
2. M1: Lexer parity slice (Rust-mirrored)
   - Reproduce token model and key lexer behavior from Rust reference.
3. M2: Parser parity slice
   - Match Rust frontend AST boundaries and precedence behavior.
4. M3: Sema core parity
   - Symbol tables, type checks, declaration/usage validation.
5. M4: IR builder (text output)
   - Temp/label allocation and control-flow emission parity with current expectations.
6. M5: Driver + preprocess wiring
   - Keep external preprocessor backend model initially (now through `Preprocessor` wrapper).
7. M6: Allowlist parity gate
   - Reach agreed pass threshold against Python baseline.
8. M7: Pure-C backport planning + first converted module
   - Start from lowest-risk module (usually lexer or utility layer).

### Test/Script Changes Required Early

- Keep runners dual-mode:
  - `.py` compiler path => `python <compiler> ...`
  - binary compiler path => `<compiler> ...`
- Ensure `scripts/full_scan.sh` and c-testsuite runner can exercise both Python and C++ frontend binaries.
- Add parity checks comparing Python vs C++ frontend behavior on focused fixtures.

### C++ Coding Constraints (for future C backport)

- `class` is allowed, but only `public` members (no `private`/`protected` sections).
- Class inheritance is forbidden.
- `virtual` is forbidden.
- Operator overloading is allowed.
- `std::vector` is allowed.
- `std::string` is allowed.
- Prefer smart pointers to mimic Rust ownership style:
  - Default to `std::unique_ptr` for single ownership.
  - Use `std::shared_ptr` only for real shared ownership needs.
  - Avoid raw `new`/`delete` in compiler core paths.
- Avoid heavy template/meta-programming in compiler core.
- Prefer explicit data structures that can be translated to C with bounded effort.
- Keep ownership/lifetime simple and documented at module boundaries.

### IR Builder Roadmap

- Phase-1 (now): string-based LLVM IR emission.
- Phase-2 (optional): LLVM API backend (`ir_builder_llvm.cpp`) behind compile-time switch.

### Main Risks

- Semantic drift from Python oracle while mirroring Rust design.
- Test harness mismatch between script-mode and binary-mode compiler.
- C++ conveniences creating backport friction.

Mitigation:

- Keep Python oracle differential tests active during each milestone.
- Lock module contracts early (token stream, AST forms, sema interfaces).
- Enforce backportable coding profile in reviews and milestone checklists.
