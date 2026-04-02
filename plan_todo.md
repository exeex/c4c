# Plan Todo

Status: Active
Source Idea: ideas/open/parser_debug_surface_for_vector_stl_work.md
Source Plan: plan.md

## Active Item

- Step 3: Mark injected token-stream parsing.

## Checklist

- [x] Inspect current parser best-failure storage and emission path.
- [x] Add targeted tests for failure-local token context output.
- [x] Implement token index, token kind, lexeme, and token-window reporting.
- [x] Re-run targeted parser/debug tests for the Step 1 slice.
- [x] Update todo state with results and the next intended slice.
- [x] Inspect tentative-parse helpers and current save/restore sites.
- [x] Add focused tests for tentative enter/commit/rollback visibility.
- [x] Implement tentative lifecycle events behind targeted debug gating.
- [x] Re-run targeted parser/debug tests for the Step 2 slice.
- [x] Inspect token-swap and injected-parse sites that bypass `TentativeParseGuard`.
- [x] Add focused coverage for injected token-stream begin/end markers at the
      template-base instantiation swap site.
- [x] Implement injected token-stream debug events around the template-base
      instantiation swap site.
- [x] Re-run targeted parser/debug tests for the Step 3 slice.

## Completed

- [x] Activated `ideas/open/parser_debug_surface_for_vector_stl_work.md` into
      `plan.md`.
- [x] Step 1 landed: parser-debug failure summaries now include token index,
      token kind, and a five-token local window on the focused negative case.
- [x] Validation: `ctest --test-dir build -R '^cpp_parser_debug_'`,
      `ctest --test-dir build -L diagnostic_format`, full `ctest -j`, and the
      monotonic before/after regression guard all passed with no new failures.
- [x] Step 2 landed: `TentativeParseGuard` now emits opt-in
      `tentative_enter`, `tentative_commit`, and `tentative_rollback` parser
      debug events with `start=`/`end=` detail, and the new
      `cpp_parser_debug_tentative_template_arg_lifecycle` regression covers the
      qualified-template ambiguity path.
- [x] Validation: `ctest --test-dir build -R '^cpp_parser_debug_'` passed after
      the tentative lifecycle work.
- [x] Step 3 slice landed at the template-base instantiation swap site:
      `parse_base_type` token injection now emits opt-in
      `injected_parse_begin` / `injected_parse_end` events, and
      `cpp_parser_debug_injected_template_base_instantiation` locks the trace.
- [x] Validation: `ctest --test-dir build -R '^cpp_parser_debug_'` passed, full
      `ctest --test-dir build -j --output-on-failure` still had only the three
      known unrelated failures, and the monotonic regression guard passed with
      `before: passed=2672 failed=3` and `after: passed=2675 failed=3`.

## Next Intended Slice

- Decide whether Step 3 needs the same begin/end markers at the remaining
  manual `tokens_` swap sites in template-member lookup and cast disambiguation
  before moving on to CLI narrowing.

## Blockers

- Broader `ctest --test-dir build -j --output-on-failure` currently reports
  unrelated non-parser failures in `backend_lir_adapter_aarch64_tests`,
  `positive_sema_linux_stage2_repro_03_asm_volatile_c`, and
  `llvm_gcc_c_torture_src_20080502_1_c`. This Step 3 slice only touched parser
  debug code and the focused parser-debug suite stayed green.

## Resume Notes

- Step 1 is complete and stayed debug-gated, so normal parser failures did not
  become noisier.
- Step 2 stayed inside the shared `TentativeParseGuard`, so speculative parse
  sites picked up lifecycle events without changing non-debug behavior.
- Step 3 now has a focused regression around the template-base instantiation
  injection path; the remaining manual `tokens_` swap sites still need a scope
  decision if we want broader injected-trace coverage.
