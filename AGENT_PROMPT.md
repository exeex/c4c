# Agent Task: tiny-c2ll C++ Frontend Handoff (2026-03-05)

## Mission

You are taking over the C++ frontend in `src/frontend_c/` and should continue improving compiler correctness with small, verifiable slices.

Primary priorities (strict order):

1. Fix known regressions first.
2. Then improve preprocessor completeness, using:
   - `ref/claudes-c-compiler/src/frontend/preprocessor/README.md`

## Current Baseline (important)

- Python frontend (`src/frontend/`) has been removed.
- Stage1 compiler binary is:
  - `build_debug/tiny-c2ll-stage1`
- Test layout now follows:
  - `tests/<test_set>/...`
  - example: `tests/tiny_c2ll/example.c`, `tests/preprocessor/frontend_cxx_preprocessor_tests.cpp`
- CMake scripts are centralized under:
  - `tests/cmake/...`
- c-testsuite/ccc-review are per-case CTest entries (parallelizable with `ctest -j`).

## First Steps (every new iteration)

1. Read `plan.md` (top sections: status, known issues, priorities).
2. Run `git status --short`.
3. Pick exactly one smallest fix slice.

## Work Loop

1. Implement one focused change in `src/frontend_c/` or test wiring.
2. Run the smallest relevant test subset.
3. If green, record notes in `build/agent_state/progress_log.md`.
4. Commit with precise scope in message.
5. Repeat.

## Hard Rules

- Do not edit `tests/c-testsuite/` vendored contents.
- Keep changes minimal and tested.
- If blocked >15 minutes, write blocker + next hypothesis to `build/agent_state/hard_bugs.md` and switch slice.
- Keep architecture close to `ref/claudes-c-compiler/src/frontend` unless there is a concrete reason.

## Immediate Known Issues To Fix First

1. `ccc-review` expected-fail gaps:
   - `tests/ccc-review-tests/0004_unicode_escape_string.c`
   - `tests/ccc-review-tests/0005_unicode_escape_char.c`
   - `tests/ccc-review-tests/0006_dollar_identifier.c`
2. c-testsuite case temporarily removed from allowlist due backend IR bug:
   - `tests/single-exec/00216.c`

Rule: whenever a gap is fixed, flip expectation / restore allowlist entry in the same PR and keep CTest green.

## Preprocessor Follow-up Focus (after above)

Use `ref/claudes-c-compiler/src/frontend/preprocessor/README.md` as the checklist.

High-value targets first:

1. Conditional-expression correctness in `#if/#elif` (macro-expanded integer expr behavior parity).
2. Macro expansion edge cases:
   - `#` stringification
   - `##` token pasting safety
   - variadic macro behavior (including empty `__VA_ARGS__` comma handling)
3. Include/path behavior parity and diagnostics quality.
4. Pragma handling parity needed by frontend parser pipeline.
5. Source location fidelity via line markers.

## Suggested Commands

```bash
cmake -S . -B build_debug
cmake --build build_debug -j8

# All tests
ctest --test-dir build_debug --output-on-failure -j 8

# Focused suites
ctest --test-dir build_debug -L ccc_review --output-on-failure -j 8
ctest --test-dir build_debug -L c_testsuite --output-on-failure -j 8
ctest --test-dir build_debug -R tiny_c2ll_tests --output-on-failure
ctest --test-dir build_debug -R frontend_cxx_preprocessor_tests --output-on-failure
```

## Exit Criteria For Each Change

- Relevant test slice passes.
- No unrelated regressions introduced.
- Clear commit with focused scope.
