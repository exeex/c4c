# Plan Todo

Status: Active
Source Idea: ideas/open/parser_debug_surface_for_vector_stl_work.md
Source Plan: plan.md

## Active Item

- Step 2: Add tentative parse lifecycle visibility.

## Checklist

- [x] Inspect current parser best-failure storage and emission path.
- [x] Add targeted tests for failure-local token context output.
- [x] Implement token index, token kind, lexeme, and token-window reporting.
- [x] Re-run targeted parser/debug tests for the Step 1 slice.
- [x] Update todo state with results and the next intended slice.
- [ ] Inspect tentative-parse helpers and current save/restore sites.
- [ ] Add focused tests for tentative enter/commit/rollback visibility.
- [ ] Implement tentative lifecycle events behind targeted debug gating.
- [ ] Re-run targeted parser/debug tests for the Step 2 slice.

## Completed

- [x] Activated `ideas/open/parser_debug_surface_for_vector_stl_work.md` into
      `plan.md`.
- [x] Step 1 landed: parser-debug failure summaries now include token index,
      token kind, and a five-token local window on the focused negative case.
- [x] Validation: `ctest --test-dir build -R '^cpp_parser_debug_'`,
      `ctest --test-dir build -L diagnostic_format`, full `ctest -j`, and the
      monotonic before/after regression guard all passed with no new failures.

## Next Intended Slice

- Start Step 2 by instrumenting the existing tentative parse guard or the
  narrowest speculative save/restore helper so `--parser-debug` can show
  `tentative_enter`, `tentative_commit`, and `tentative_rollback` without
  widening the default trace surface.

## Blockers

- None recorded.

## Resume Notes

- Step 1 is complete and stayed debug-gated, so normal parser failures did not
  become noisier.
- Reuse the parser-debug negative-case harness for Step 2 if possible rather
  than introducing a separate test driver.
