# tiny-c2ll Plan (Handoff Snapshot)

Last updated: 2026-03-05

## Current State

- Active compiler frontend is C++ in `src/frontend_c/`.
- Python frontend (`src/frontend/`) has been removed.
- CMake/CTest now drives all test suites.
- Tests are organized by set under `tests/<test_set>/...`.
- CMake helper scripts are centralized in `tests/cmake/...`.

## Build and Test Baseline

```bash
cmake -S . -B build_debug
cmake --build build_debug -j8
ctest --test-dir build_debug --output-on-failure -j 8
```

Current baseline from latest run:
- `233/233` tests passed.

## Test Topology

- tiny regression:
  - `tiny_c2ll_tests` (CMake script: `tests/cmake/run_tiny_c2ll_tests.cmake`)
- preprocessor unit tests:
  - `frontend_cxx_preprocessor_tests`
- ccc review (per-case CTest, label `ccc_review`):
  - `tests/ccc-review-tests/*.c`
- c-testsuite allowlist (per-case CTest, label `c_testsuite`):
  - entries from `tests/c_testsuite_allowlist.txt`

## Known Issues (Priority: Fix First)

1. `ccc-review` expected-fail cases still open:
   - `0004_unicode_escape_string.c`
   - `0005_unicode_escape_char.c`
   - `0006_dollar_identifier.c`
2. c-testsuite known backend bug case is excluded from allowlist:
   - `tests/single-exec/00216.c`

### Fix Policy

- For each fixed issue, update tests in same change:
  - Flip `CCC_EXPECT` fail -> pass when appropriate.
  - Re-add `00216.c` to allowlist once backend IR is corrected.
- Keep full CTest green after each change.

## Main Roadmap (Near Term)

### Phase A: Close Known Gaps

1. Fix Unicode string/char literal semantics end-to-end.
2. Fix `$` identifier behavior compatibility (or produce explicit, tested policy).
3. Fix backend IR correctness for `00216.c` aggregate/global lowering.

### Phase B: Preprocessor Completion

Reference:
- `ref/claudes-c-compiler/src/frontend/preprocessor/README.md`

Target areas (ordered):

1. `#if/#elif` expression parity and correctness.
2. Macro expansion edge cases:
   - stringify `#`
   - paste `##`
   - variadic + empty `__VA_ARGS__` handling
3. Include search path behavior and diagnostics.
4. Pragmas used by frontend/parser integration.
5. Stable line-marker/source-location fidelity.

## Execution Rules For Next Agent

1. One smallest slice per commit.
2. Always run smallest relevant tests first, then broaden.
3. No edits inside vendored `tests/c-testsuite/`.
4. If blocked >15 min, log blocker in `build/agent_state/hard_bugs.md` and switch slice.

## Useful Commands

```bash
# Build
cmake -S . -B build_debug
cmake --build build_debug -j8

# Full suite
ctest --test-dir build_debug --output-on-failure -j 8

# Focused suites
ctest --test-dir build_debug -L ccc_review --output-on-failure -j 8
ctest --test-dir build_debug -L c_testsuite --output-on-failure -j 8
ctest --test-dir build_debug -R tiny_c2ll_tests --output-on-failure
ctest --test-dir build_debug -R frontend_cxx_preprocessor_tests --output-on-failure

# Convenience wrappers
bash scripts/check_progress.sh
bash scripts/full_scan.sh
```

## Recent Migration Notes (Already Done)

- Moved from Python runners to CMake script runners.
- Reorganized tests into:
  - `tests/tiny_c2ll/`
  - `tests/preprocessor/`
  - `tests/ccc-review-tests/`
  - `tests/c-testsuite/`
  - `tests/cmake/`

