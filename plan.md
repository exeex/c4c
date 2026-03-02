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
  - temporary external preprocessing via `clang -E -P`
- End-to-end test pipeline is in place:
  - `c2ll.py -> .ll -> clang -> executable`
  - failure logs split into frontend/backend/runtime
- c-testsuite status:
  - Allowlist run (`2026-02-28`): `209 / 218` pass, `9` fail
  - Regression cases:
    - Frontend: `tests/single-exec/00215.c`
    - Backend: `tests/single-exec/00040.c`, `00134.c`, `00181.c`, `00182.c`, `00189.c`, `00200.c`, `00217.c`
    - Runtime: `tests/single-exec/00104.c`
  - `tests/c_testsuite_allowlist.txt` currently tracks 218 selected cases
- Local regression status (`ctest --test-dir build --output-on-failure`, `2026-02-28`):
  - `c_testsuite_allowlist`: fail (`209 / 218` pass)
  - `tiny_c2ll_tests`: pass

## Next Steps (Priority Order)

1. Fix backend regressions introduced by the latest ABI/initializer/type changes (`00040`, `00134`, `00181`, `00182`, `00189`, `00200`, `00217`).
2. Fix the runtime regression in `tests/single-exec/00104.c`.
3. Fix the frontend regression in `tests/single-exec/00215.c`.
4. Re-run allowlist and only expand/commit milestones when the full allowlist is green.

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
  - `--compiler src/c2ll.py`
  - (runner flow is: Clang compile/run baseline first, then `python src/c2ll.py` -> `.ll` -> Clang -> run)

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
cmake -S . -B build
cmake --build build
```

### Run all configured tests

```bash
ctest --test-dir build --output-on-failure
```

### Run only ccc review tests

```bash
ctest --test-dir build -R ccc_review_tests --output-on-failure
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

## C Rewrite Frontend Plan (Self-Hosting, LLVM IR Text Output First)

### Target

- Rewrite `src/frontend/*.py` in C (allow limited C++ helpers in `utils.cpp`).
- Keep output as LLVM IR text (debug-friendly).
- Reach self-compilation (bootstrap) with stable test pipeline.

### Reference Mapping

- `/tmp/ref/amacc`: take minimalist single-pass style, explicit symbol tables, precedence parsing mindset.
- `/tmp/ref/claudes-c-compiler`: take strict phase boundaries and module contracts (`frontend -> sema -> IR`), plus differential testing discipline.
- Current Python frontend remains stage0 oracle for behavior parity while migrating.

### Proposed Source Layout

- `src/frontend_c/`
  - `common.h` `diag.c` (errors, span)
  - `token.h` `lexer.c`
  - `ast.h` `arena.c`
  - `parser.c`
  - `sema.c`
  - `ir_builder.c` (string emitter)
  - `driver.c` (`main`, `-o`, preprocess call)
  - `utils.h` `utils.cpp` (complex string/path utilities only, C ABI)

### Bootstrap Strategy

1. Stage0
   - Existing Python compiler compiles C frontend sources to `.ll`.
2. Stage1
   - `clang` links generated `.ll` into native compiler binary.
3. Stage2
   - New C compiler compiles its own sources again.
   - Compare stage1/stage2 behavior on smoke + allowlist tests.

### Milestones

1. M0: Build skeleton + CMake targets
   - Add `frontend_c_stage1` executable target.
   - Keep Python path untouched.
2. M1: Lexer + parser bootstrap subset
   - Cover syntax actually used by `frontend_c` source first.
3. M2: Semantic analyzer core
   - Symbol tables, type checks, init/use checks, function signature checks.
4. M3: IR builder (text output)
   - Temp/label allocator, LLVM type mapping, control-flow emission.
5. M4: Driver + preprocess
   - Keep `clang -E -P` preprocess model initially (same as Python).
6. M5: Self-host smoke
   - Compile frontend C source with frontend C compiler itself.
7. M6: c-testsuite allowlist parity
   - Catch up to Python baseline, then flip default compiler path.

### Test/Script Changes Required Early

- Existing test runners call compiler as Python script.
- Make runners dual-mode:
  - `.py` path => `python <compiler> ...`
  - binary path => `<compiler> ...`
- This is required for `scripts/full_scan.sh` to run C frontend binary directly.

### `utils.cpp` Rule

- Allowed only for complex helpers (string formatting, path normalization, reusable buffer ops).
- Export only C ABI entry points (`extern "C"`), keep parser/sema/IR core in C for self-host transparency.

### IR Builder Roadmap

- Phase-1 (now): string-based LLVM IR emission.
- Phase-2 (optional): separate `ir_builder_llvm.cpp` with LLVM API backend behind a compile-time switch.

### Main Risks

- Feature gap between Python frontend and bootstrap C subset.
- Harness assumes Python invocation.
- Semantic drift during migration.

Mitigation:

- Freeze coding profile for `src/frontend_c` until stage2 passes.
- Keep Python as oracle and run differential checks on token/AST/IR snippets.
- Add binary-mode test support before deep migration.
