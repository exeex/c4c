# Parser Error Diagnostics Todo

Status: Active
Source Idea: ideas/open/parser_error_diagnostics_plan.md
Source Plan: plan.md

## Active Item

- Step 5: prepare the next diagnostic slice by bounding the first
  committed-failure vs no-match follow-up under speculative `try_parse_*`
  record-member rewinds
- Current slice: bound the next wrapper-heavy top-level qualified-type case
  after the completed alias/reference-parameter `got='&'` ranking change
- Iteration target: reduce and compare the remaining
  `/usr/include/c++/14/bits/stl_bvector.h:663` `got='&&'` family, where a
  wrapper-heavy top-level failure still summarizes
  `try_parse_qualified_base_type`, and decide whether that path should move to
  an outer committed wrapper or keep the current deeper leaf

## Completed

- recorded the required clean after-suite for this iteration and passed the
  monotonic regression guard against the recorded
  `before passed=2281/2282` baseline:
  `after passed=2281/2282`; the existing
  `verify_tests_verify_top_level_recovery` failure remained unchanged and the
  guard script reported zero new failing tests
- rebaselined the active reduced alias-reference parser-debug case and the
  motivating `tests/cpp/std/std_vector_simple.cpp`
  `/usr/include/c++/14/bits/exception.h:65` path, then confirmed both traces
  retain the same qualified-type stack while the committed failure point is
  the outer `parse_top_level_parameter_list`
- tightened `select_best_parse_summary_leaf()` so
  `parse_top_level_parameter_list` now wins the emitted `parse_fn=...` summary
  when the remaining suffix is only `parse_param` plus qualified-type probe
  frames, preserving the deeper debug stack for reduction work
- updated focused parser-debug regression coverage in
  `cpp_parser_debug_std_vector_ref_param_leaf` and
  `cpp_parser_debug_qualified_alias_ref_param_leaf` to lock in the outer
  committed parameter-list summary for the shared `got='&'` family while
  keeping the existing stack substrings
- reran focused parser-debug coverage for
  `cpp_parser_debug_qualified_type_top_level_params`,
  `cpp_parser_debug_std_vector_ref_param_leaf`,
  `cpp_parser_debug_qualified_alias_ref_param_leaf`, and
  `cpp_parser_debug_top_level_qualified_probe_leaf`
- recorded the required clean after-suite for this iteration and passed the
  monotonic regression guard against the recorded
  `before passed=2279/2280` baseline:
  `after passed=2281/2282`; the existing
  `verify_tests_verify_top_level_recovery` failure remained unchanged
- reduced the `std_vector_simple.cpp` `got='&'` reference-parameter family
  into standalone parser-debug coverage with
  `cpp_parser_debug_qualified_alias_ref_param_leaf`, using a
  namespace-scoped template alias referenced unqualified in
  `parser_debug_qualified_alias_ref_param_leaf.cpp` to preserve the same
  committed `parse_fn=try_parse_qualified_base_type` summary and
  `parse_top_level -> try_parse_cpp_scoped_base_type ->
  consume_qualified_type_spelling -> parse_top_level_parameter_list ->
  parse_param -> try_parse_cpp_scoped_base_type ->
  try_parse_qualified_base_type` stack family without depending on
  `tests/cpp/std/std_vector_simple.cpp`
- reran focused parser-debug coverage for
  `cpp_parser_debug_qualified_type_top_level_params`,
  `cpp_parser_debug_std_vector_ref_param_leaf`,
  `cpp_parser_debug_qualified_alias_ref_param_leaf`, and
  `cpp_parser_debug_top_level_qualified_probe_leaf`
- recorded the next Step 5 tri-state question: decide whether this
  alias-or-qualified reference-parameter family should eventually rank the
  outer committed `parse_top_level_parameter_list` failure ahead of the
  retained inner `try_parse_qualified_base_type` leaf, or keep the current
  deeper-leaf summary while preserving the wrapper context in debug mode
- recorded the required clean after-suite for this iteration and passed the
  monotonic regression guard against the recorded
  `before passed=2278/2279` baseline:
  `after passed=2279/2280`; the existing
  `verify_tests_verify_top_level_recovery` failure remained unchanged
- sampled the remaining `tests/cpp/std/std_vector_simple.cpp`
  parser-debug output again and bounded the next distinct speculative
  `try_parse_*` candidate around `/usr/include/c++/14/bits/exception.h:65`,
  where a reference-qualified top-level parameter path reports
  `parse_fn=try_parse_qualified_base_type` with `got='&'`
- added focused parser-debug coverage in
  `cpp_parser_debug_std_vector_ref_param_leaf` so the motivating
  `std_vector_simple.cpp` trace now locks the `exception.h:65`
  summary/stack shape:
  `parse_top_level -> consume_qualified_type_spelling ->
  parse_top_level_parameter_list -> parse_param ->
  try_parse_cpp_scoped_base_type -> try_parse_qualified_base_type`
- reran focused parser-debug coverage for
  `cpp_parser_debug_std_vector_wrapper_spelling_stack`,
  `cpp_parser_debug_std_vector_wrapper_anchor_stack`,
  `cpp_parser_debug_std_vector_ref_param_leaf`, and
  `cpp_parser_debug_top_level_qualified_probe_leaf`
- recorded the required clean after-suite for this iteration and passed the
  monotonic regression guard against the recorded
  `before passed=2274/2275` baseline:
  `after passed=2278/2279`; the existing
  `verify_tests_verify_top_level_recovery` failure remained unchanged
- added parser-debug regression coverage in
  `cpp_parser_debug_std_vector_wrapper_spelling_stack` and
  `cpp_parser_debug_std_vector_wrapper_anchor_stack` against the motivating
  `tests/cpp/std/std_vector_simple.cpp` wrapper-only top-level failures
- generalized the top-level qualified-probe summary merge so
  `best_debug_summary_stack()` now prepends leading
  `consume_qualified_type_spelling` / `try_parse_cpp_scoped_base_type` frames
  when the emitted summary would otherwise jump straight from
  `parse_top_level` into `parse_top_level_parameter_list`
- rechecked the motivating `std_vector_simple.cpp` parser-debug output after
  landing this slice; `/usr/include/c++/14/bits/exception.h:67`,
  `/usr/include/c++/14/bits/stl_vector.h:336` / `:340`, and
  `/usr/include/c++/14/bits/stl_bvector.h:78` now keep their leading
  qualified-type probe frames in the emitted summary stack
- reran focused parser-debug coverage for
  `cpp_parser_debug_qualified_type_top_level_params`,
  `cpp_parser_debug_qualified_type_template_arg_stack`,
  `cpp_parser_debug_qualified_type_dependent_typename_stack`,
  `cpp_parser_debug_qualified_type_typename_spelling_stack`,
  `cpp_parser_debug_qualified_type_spelling_stack`,
  `cpp_parser_debug_std_vector_wrapper_spelling_stack`,
  `cpp_parser_debug_std_vector_wrapper_anchor_stack`, and
  `cpp_parser_debug_top_level_qualified_probe_leaf`
- recorded the required clean after-suite for this slice and passed the
  monotonic regression guard against the recorded
  `before passed=2274/2275` baseline on rerun:
  `after passed=2276/2277`; the existing
  `verify_tests_verify_top_level_recovery` failure remained unchanged
- recorded the required clean full-suite baseline for this slice:
  `before passed=2276/2277`; the existing
  `verify_tests_verify_top_level_recovery` failure remained unchanged
- tightened `cpp_parser_debug_qualified_type_template_arg_stack` so the test
  now asserts the full stack substring instead of a weak tokenized `[pdebug]`
  fragment, then updated it to require the leading top-level qualified probe
  frames before the nested template-argument path
- updated `best_debug_summary_stack()` to prepend the leading top-level
  qualified-type probe segment from the parser debug event stream when the
  emitted summary would otherwise jump straight from `parse_top_level` into
  `parse_next_template_argument`
- updated nearby reduced coverage in
  `cpp_parser_debug_qualified_type_spelling_stack` to lock in the same
  prefixed top-level qualified probe summary shape
- reran focused parser-debug coverage for
  `cpp_parser_debug_expr_stmt_stack`,
  `cpp_parser_debug_record_member_stack`,
  `cpp_parser_debug_record_member_param_default_rank`,
  `cpp_parser_debug_record_member_type_like_rank`,
  `cpp_parser_debug_record_member_using_alias_leaf`,
  `cpp_parser_debug_record_member_typedef_leaf`,
  `cpp_parser_debug_qualified_type_top_level_params`,
  `cpp_parser_debug_qualified_type_template_arg_stack`,
  `cpp_parser_debug_qualified_type_dependent_typename_stack`,
  `cpp_parser_debug_qualified_type_typename_spelling_stack`,
  `cpp_parser_debug_qualified_type_spelling_stack`,
  `cpp_parser_debug_if_init_qualified_probe_leaf`, and
  `cpp_parser_debug_top_level_qualified_probe_leaf`
- reran the required clean before/after full suite and passed the monotonic
  regression guard with no new failures:
  `before passed=2276/2277`, `after passed=2276/2277`; the existing
  `verify_tests_verify_top_level_recovery` failure remained unchanged
- rechecked the motivating `std_vector_simple.cpp` parser-debug output after
  landing this slice; the qualified-template-argument summary stacks now keep
  their leading top-level probe frames, while the separate
  `consume_qualified_type_spelling`-only wrapper cases at
  `exception.h:67`, `stl_vector.h:336` / `:340`, and `stl_bvector.h:78`
  remain as the next bounded follow-up
- noted that the first clean full-suite run transiently timed out
  `llvm_gcc_c_torture_src_pr28982b_c` under parallel load, then revalidated it
  in isolation (`Passed 7.14 sec`) before rerunning the full suite to confirm
  there was no persistent new failure
- reduced the `/usr/include/c++/14/bits/stl_bvector.h:663` wrapper-heavy move
  constructor failure into
  `cpp_parser_debug_top_level_qualified_probe_leaf`, using a nested
  `requires`-preceded record member to preserve the same bad
  `parse_fn=parse_top_level` summary shape in a standalone parser-debug case
- tightened `select_best_parse_summary_leaf()` so
  `parse_top_level` no longer wins the emitted `parse_fn=...` summary when the
  trailing summary stack consists only of speculative qualified-type probe
  frames
- added reduced parser-debug regression coverage in
  `cpp_parser_debug_top_level_qualified_probe_leaf` to lock in the deeper
  `try_parse_qualified_base_type` summary leaf while preserving the existing
  top-level qualified-probe stack line
- reran focused parser-debug coverage for
  `cpp_parser_debug_top_level_qualified_probe_leaf`,
  `cpp_parser_debug_if_init_qualified_probe_leaf`,
  `cpp_parser_debug_qualified_type_top_level_params`,
  `cpp_parser_debug_qualified_type_template_arg_stack`,
  `cpp_parser_debug_qualified_type_dependent_typename_stack`,
  `cpp_parser_debug_qualified_type_typename_spelling_stack`, and
  `cpp_parser_debug_qualified_type_spelling_stack`
- recorded the required full-suite baseline for this iteration:
  `before passed=2274/2275`; the existing
  `verify_tests_verify_top_level_recovery` failure remained unchanged
- confirmed `HEAD` already contains the reduced
  `cpp_parser_debug_if_init_qualified_probe_leaf` coverage plus the committed
  leaf-selection fix, and revalidated that targeted parser-debug repro on the
  current tree before choosing a new Step 5 slice
- re-ran the recorded next-candidate parser-debug repros in
  `cpp_parser_debug_qualified_type_typename_spelling_stack` and
  `cpp_parser_debug_qualified_type_spelling_stack`; both still show the
  expected committed `parse_top_level_parameter_list` summary with the richer
  speculative qualified-type stack preserved in debug mode, so they are not
  the next ranking bug
- sampled the active `std_vector_simple.cpp` parser-debug output and bounded
  the next wrapper-heavy candidate around
  `/usr/include/c++/14/bits/stl_bvector.h:663`, where a member-definition
  failure still reports `parse_fn=parse_top_level` even though the debug event
  stream preserves the surrounding qualified-type helper path
- parked the adjacent parser-diagnostics instrumentation work as its own idea:
  `ideas/open/parser_error_diagnostics_plan.md`
- added an initial parser debug path gated by `--parser-debug`
- added parser-side failure/debug state in
  `src/frontend/parser/parser.hpp` / `src/frontend/parser/parse.cpp`
- switched the trace model away from hand-written context labels toward parse
  function names
- replaced the temporary macro approach with direct
  `ParseContextGuard trace(this, __func__)` call sites
- validated that `c4cll` still builds after the first instrumentation slice
- added guard coverage for the remaining high-signal statement, declaration,
  and expression entry points: `parse_block`, `parse_stmt`,
  `parse_local_decl`, `parse_top_level`, `parse_assign_expr`,
  `parse_ternary`, `parse_unary`, `parse_initializer`, and `parse_init_list`
- added reduced parser-debug regression coverage in
  `cpp_parser_debug_expr_stmt_stack` for the short error line plus stable
  stack snapshot
- taught statement-level block recovery to emit the best parse-failure summary
  and parser-debug trace instead of dropping back to raw `e.what()`
- recorded clean before/after suite logs and passed the monotonic regression
  guard:
  `before passed=2250/2251`, `after passed=2252/2253`, no new failing tests
- collapsed duplicate adjacent frames in the emitted best-failure
  `[pdebug] stack:` summary so recursive expression failures keep the full event
  log but a less repetitive stack line
- updated `cpp_parser_debug_expr_stmt_stack` to lock in the compact summary
  stack shape
- reran the required clean before/after full suite for this slice and passed
  the monotonic regression guard with no new failures:
  `before passed=2252/2253`, `after passed=2252/2253`
- added reduced parser-debug regression coverage in
  `cpp_parser_debug_record_member_stack` for a malformed nested record member
  that rewinds through `try_parse_record_member_dispatch`
- taught parser debug summaries to retain the furthest/deepest recorded parse
  stack when a speculative record-member rewind would otherwise collapse the
  final error back to `parse_top_level`
- kept adjacent duplicate-frame compaction on the summary stack while letting
  record-member failures surface `try_parse_record_method_or_field_member` as
  the leaf parse function in debug summaries
- reran focused record-member/parser-debug coverage plus the required clean
  before/after full suite and passed the monotonic regression guard:
  `before passed=2252/2253`, `after passed=2254/2255`, no new failing tests
- added reduced parser-debug regression coverage in
  `cpp_parser_debug_record_member_param_default_rank` for a malformed record
  member parameter default that previously degraded from the inner
  `expected=RPAREN` failure to an outer `expected=RBRACE` wrapper error
- updated best parse-failure ranking so an adjacent-token committed wrapper
  unwind does not replace a deeper committed inner failure when the wrapper
  stack is only a strict prefix of the earlier failure
- reran focused parser-debug coverage for
  `cpp_parser_debug_expr_stmt_stack`,
  `cpp_parser_debug_record_member_stack`, and
  `cpp_parser_debug_record_member_param_default_rank`
- reran the required clean before/after full suite and passed the monotonic
  regression guard with no new failing tests:
  `before passed=2254/2255`, `after passed=2256/2257`; the existing
  `verify_tests_verify_top_level_recovery` failure remained unchanged
- added `ParseContextGuard` coverage to
  `try_parse_record_type_like_member_dispatch` so parser-debug summaries keep
  the type-like record-member dispatch frame instead of collapsing back to the
  outer `try_parse_record_member_dispatch` wrapper
- added reduced parser-debug regression coverage in
  `cpp_parser_debug_record_member_type_like_rank` for a malformed record-body
  alias target that now keeps the type-like dispatch leaf in the error summary
  and stack line
- reran focused parser-debug coverage for
  `cpp_parser_debug_record_member_stack`,
  `cpp_parser_debug_record_member_param_default_rank`, and
  `cpp_parser_debug_record_member_type_like_rank`
- reran nearby record-member parser coverage for
  `cpp_positive_sema_record_member_dispatch_parse_cpp`,
  `cpp_positive_sema_record_member_enum_parse_cpp`,
  `cpp_positive_sema_record_member_type_dispatch_parse_cpp`, and
  `cpp_positive_sema_record_member_typedef_using_parse_cpp`
- reran the required clean before/after full suite and passed the monotonic
  regression guard with no new failures:
  `before passed=2256/2257`, `after passed=2258/2259`; the existing
  `verify_tests_verify_top_level_recovery` failure remained unchanged
- added `ParseContextGuard` coverage to `try_parse_record_using_member` and
  `try_parse_record_typedef_member`, and tightened both parsers to require
  their terminating semicolon with `expect(TokenKind::Semi)` so malformed
  alias/typedef members report the deeper local parser leaf
- updated `cpp_parser_debug_record_member_type_like_rank` to assert the new
  leaf-local summary shape while preserving the type-like dispatch frame in the
  stack line
- added reduced parser-debug regressions in
  `cpp_parser_debug_record_member_using_alias_leaf` and
  `cpp_parser_debug_record_member_typedef_leaf` for malformed record type-like
  members that previously collapsed to the outer wrapper-only root cause
- reran focused parser-debug coverage for
  `cpp_parser_debug_record_member_stack`,
  `cpp_parser_debug_record_member_param_default_rank`,
  `cpp_parser_debug_record_member_type_like_rank`,
  `cpp_parser_debug_record_member_using_alias_leaf`, and
  `cpp_parser_debug_record_member_typedef_leaf`
- reran nearby record-member parser coverage for
  `cpp_positive_sema_record_member_dispatch_parse_cpp`,
  `cpp_positive_sema_record_member_enum_parse_cpp`,
  `cpp_positive_sema_record_member_type_dispatch_parse_cpp`, and
  `cpp_positive_sema_record_member_typedef_using_parse_cpp`
- reran the required clean before/after full suite and passed the monotonic
  regression guard with no new failures:
  `before passed=2258/2259`, `after passed=2262/2263`; the existing
  `verify_tests_verify_top_level_recovery` failure remained unchanged
- recorded the required full-suite baseline for this iteration:
  `before passed=2262/2263`; the existing
  `verify_tests_verify_top_level_recovery` failure remained unchanged
- added reduced parser-debug regression coverage in
  `cpp_parser_debug_qualified_type_top_level_params` for a malformed
  namespace-qualified top-level declaration whose unterminated parameter list
  previously collapsed back to `parse_top_level`
- extracted the top-level function/declaration parameter parser into
  `parse_top_level_parameter_list()` so parser-debug summaries retain the
  deeper local leaf for this file-scope declaration path
- added `ParseContextGuard` coverage to `try_parse_cpp_scoped_base_type` and
  `try_parse_qualified_base_type`, then tightened parser-debug summary-stack
  selection so the top-level parameter-list wrapper preserves the preceding
  qualified-type probe frames without regressing existing record-member
  summaries
- updated `cpp_parser_debug_qualified_type_top_level_params` to assert the
  merged stack shape
- reran focused parser-debug coverage for
  `cpp_parser_debug_expr_stmt_stack`,
  `cpp_parser_debug_record_member_stack`,
  `cpp_parser_debug_record_member_param_default_rank`,
  `cpp_parser_debug_record_member_type_like_rank`,
  `cpp_parser_debug_record_member_using_alias_leaf`,
  `cpp_parser_debug_record_member_typedef_leaf`, and
  `cpp_parser_debug_qualified_type_top_level_params`
- reran the required clean before/after full suite and passed the monotonic
  regression guard with no new failures:
  `before passed=2262/2263`, `after passed=2264/2265`; the existing
  `verify_tests_verify_top_level_recovery` failure remained unchanged
- recorded the required clean after-suite for this iteration and passed the
  monotonic regression guard with no new failures:
  `before passed=2264/2265`, `after passed=2264/2265`; the existing
  `verify_tests_verify_top_level_recovery` failure remained unchanged
- added reduced parser-debug regression coverage in
  `cpp_parser_debug_qualified_type_template_arg_stack` for a malformed
  namespace-qualified template type declarator whose later top-level parameter
  failure previously collapsed the summary stack back to wrapper-only frames
- tightened top-level wrapper snapshot preservation so qualified-type probe
  frames captured under `parse_next_template_argument` survive later
  `parse_top_level_parameter_list` / `parse_top_level` unwinds and reappear in
  the emitted parser-debug summary stack
- reran focused parser-debug coverage for
  `cpp_parser_debug_expr_stmt_stack`,
  `cpp_parser_debug_record_member_stack`,
  `cpp_parser_debug_record_member_param_default_rank`,
  `cpp_parser_debug_record_member_type_like_rank`,
  `cpp_parser_debug_record_member_using_alias_leaf`,
  `cpp_parser_debug_record_member_typedef_leaf`,
  `cpp_parser_debug_qualified_type_top_level_params`, and
  `cpp_parser_debug_qualified_type_template_arg_stack`
- reran the required clean before/after full suite and passed the monotonic
  regression guard with no new failures:
  `before passed=2264/2265`, `after passed=2266/2267`; the existing
  `verify_tests_verify_top_level_recovery` failure remained unchanged
- added reduced parser-debug regression coverage in
  `cpp_parser_debug_qualified_type_dependent_typename_stack` for a malformed
  dependent-typename template argument whose later top-level parameter failure
  previously kept `try_parse_cpp_scoped_base_type` but dropped the local
  `parse_dependent_typename_specifier` helper from the emitted summary stack
- added `ParseContextGuard` coverage to
  `parse_dependent_typename_specifier` so dependent-typename qualified-type
  probes retain their local helper frame in parser-debug summaries
- reran focused parser-debug coverage for
  `cpp_parser_debug_expr_stmt_stack`,
  `cpp_parser_debug_record_member_stack`,
  `cpp_parser_debug_record_member_param_default_rank`,
  `cpp_parser_debug_record_member_type_like_rank`,
  `cpp_parser_debug_record_member_using_alias_leaf`,
  `cpp_parser_debug_record_member_typedef_leaf`,
  `cpp_parser_debug_qualified_type_top_level_params`,
  `cpp_parser_debug_qualified_type_template_arg_stack`, and
  `cpp_parser_debug_qualified_type_dependent_typename_stack`
- reran nearby qualified-type parser coverage for
  `cpp_positive_sema_qualified_dependent_typename_global_parse_cpp`,
  `cpp_positive_sema_qualified_type_resolution_dispatch_parse_cpp`,
  `cpp_positive_sema_qualified_type_spelling_shared_parse_cpp`,
  `cpp_positive_sema_qualified_type_start_probe_parse_cpp`, and
  `cpp_positive_sema_qualified_type_start_shared_probe_parse_cpp`
- reran the required clean before/after full suite and passed the monotonic
  regression guard with no new failures:
  `before passed=2266/2267`, `after passed=2268/2269`; the existing
  `verify_tests_verify_top_level_recovery` failure remained unchanged
- added reduced parser-debug regression coverage in
  `cpp_parser_debug_qualified_type_typename_spelling_stack` for a nested
  dependent `typename` template argument whose summary stack previously kept
  `parse_dependent_typename_specifier` but not the local
  `consume_qualified_type_spelling_with_typename` helper
- added `ParseContextGuard` coverage to
  `consume_qualified_type_spelling_with_typename` so nested dependent
  qualified-type spelling remains visible in parser-debug summary stacks
- reran focused parser-debug coverage for
  `cpp_parser_debug_qualified_type_top_level_params`,
  `cpp_parser_debug_qualified_type_template_arg_stack`,
  `cpp_parser_debug_qualified_type_dependent_typename_stack`, and
  `cpp_parser_debug_qualified_type_typename_spelling_stack`
- reran nearby qualified-type parser coverage for
  `cpp_positive_sema_qualified_dependent_typename_global_parse_cpp`,
  `cpp_positive_sema_qualified_type_resolution_dispatch_parse_cpp`,
  `cpp_positive_sema_qualified_type_spelling_shared_parse_cpp`,
  `cpp_positive_sema_qualified_type_start_probe_parse_cpp`, and
  `cpp_positive_sema_qualified_type_start_shared_probe_parse_cpp`
- reran the required clean before/after full suite and passed the monotonic
  regression guard with no new failures:
  `before passed=2268/2269`, `after passed=2270/2271`; the existing
  `verify_tests_verify_top_level_recovery` failure remained unchanged
- recorded the required full-suite baseline for this iteration:
  `before passed=2270/2271`; the existing
  `verify_tests_verify_top_level_recovery` failure remained unchanged
- added reduced parser-debug regression coverage in
  `cpp_parser_debug_qualified_type_spelling_stack` for a nested qualified
  template argument without `typename` whose summary stack previously dropped
  the fully spelled `consume_qualified_type_spelling` helper path and
  collapsed back to wrapper-only frames
- added `ParseContextGuard` coverage to
  `consume_qualified_type_spelling` so non-`typename` qualified-name spelling
  remains visible in parser-debug event streams and summary stacks
- tightened parse-debug snapshot preservation so strict wrapper-prefix unwinds
  do not replace a deeper recorded stack with a shallower outer wrapper during
  record-member or qualified-type rewinds
- kept helper-only qualified-type frames out of the summary `parse_fn=...`
  leaf selection so existing record-member diagnostics continue to point at
  the owning parser entry point while the stack line still shows the deeper
  helper path
- reran focused parser-debug coverage for
  `cpp_parser_debug_expr_stmt_stack`,
  `cpp_parser_debug_record_member_stack`,
  `cpp_parser_debug_record_member_param_default_rank`,
  `cpp_parser_debug_record_member_type_like_rank`,
  `cpp_parser_debug_record_member_using_alias_leaf`,
  `cpp_parser_debug_record_member_typedef_leaf`,
  `cpp_parser_debug_qualified_type_top_level_params`,
  `cpp_parser_debug_qualified_type_template_arg_stack`,
  `cpp_parser_debug_qualified_type_dependent_typename_stack`,
  `cpp_parser_debug_qualified_type_typename_spelling_stack`, and
  `cpp_parser_debug_qualified_type_spelling_stack`
- reran nearby qualified-type and record-member parser coverage for
  `cpp_positive_sema_qualified_dependent_typename_global_parse_cpp`,
  `cpp_positive_sema_qualified_type_resolution_dispatch_parse_cpp`,
  `cpp_positive_sema_qualified_type_spelling_shared_parse_cpp`,
  `cpp_positive_sema_qualified_type_start_probe_parse_cpp`,
  `cpp_positive_sema_qualified_type_start_shared_probe_parse_cpp`,
  `cpp_positive_sema_record_member_dispatch_parse_cpp`,
  `cpp_positive_sema_record_member_enum_parse_cpp`,
  `cpp_positive_sema_record_member_type_dispatch_parse_cpp`, and
  `cpp_positive_sema_record_member_typedef_using_parse_cpp`
- reran the required clean before/after full suite and passed the monotonic
  regression guard with no new failures:
  `before passed=2270/2271`, `after passed=2272/2273`; the existing
  `verify_tests_verify_top_level_recovery` failure remained unchanged
- added `ParseContextGuard` coverage to `try_parse_template_type_arg` so
  parser-debug summaries can retain the speculative template-type dispatch
  frame when qualified template arguments later unwind into top-level wrapper
  failures
- updated the reduced parser-debug regressions in
  `cpp_parser_debug_qualified_type_template_arg_stack`,
  `cpp_parser_debug_qualified_type_dependent_typename_stack`, and
  `cpp_parser_debug_qualified_type_typename_spelling_stack` to lock in the new
  `try_parse_template_type_arg` frame while leaving
  `cpp_parser_debug_qualified_type_spelling_stack` on its existing collapsed
  wrapper-only summary shape
- reran focused parser-debug coverage for
  `cpp_parser_debug_qualified_type_top_level_params`,
  `cpp_parser_debug_qualified_type_template_arg_stack`,
  `cpp_parser_debug_qualified_type_dependent_typename_stack`,
  `cpp_parser_debug_qualified_type_typename_spelling_stack`, and
  `cpp_parser_debug_qualified_type_spelling_stack`
- reran nearby qualified-type parser coverage for
  `cpp_positive_sema_qualified_dependent_typename_global_parse_cpp`,
  `cpp_positive_sema_qualified_type_resolution_dispatch_parse_cpp`,
  `cpp_positive_sema_qualified_type_spelling_shared_parse_cpp`,
  `cpp_positive_sema_qualified_type_start_probe_parse_cpp`, and
  `cpp_positive_sema_qualified_type_start_shared_probe_parse_cpp`
- reran the required clean before/after full suite and passed the monotonic
  regression guard with no new failures:
  `before passed=2272/2273`, `after passed=2272/2273`; the existing
  `verify_tests_verify_top_level_recovery` failure remained unchanged
- recorded the required full-suite baseline for this iteration:
  `before passed=2272/2273`; the existing
  `verify_tests_verify_top_level_recovery` failure remained unchanged
- added reduced parser-debug regression coverage in
  `cpp_parser_debug_qualified_type_spelling_stack` for a nested qualified
  template argument without `typename` whose summary stack now preserves the
  local speculative `parse_next_template_argument` /
  `try_parse_template_type_arg` path instead of collapsing to wrapper-only
  frames
- tightened qualified-type summary snapshot preservation so a saved speculative
  template-type stack anchored at `try_parse_template_type_arg` survives later
  helper and top-level wrapper events until the committed
  `parse_top_level_parameter_list` failure is reported
- reran focused parser-debug coverage for
  `cpp_parser_debug_qualified_type_spelling_stack`
- reran nearby qualified-type parser/debug coverage for
  `cpp_parser_debug_qualified_type_top_level_params`,
  `cpp_parser_debug_qualified_type_template_arg_stack`,
  `cpp_parser_debug_qualified_type_dependent_typename_stack`,
  `cpp_parser_debug_qualified_type_typename_spelling_stack`,
  `cpp_positive_sema_qualified_dependent_typename_global_parse_cpp`,
  `cpp_positive_sema_qualified_type_resolution_dispatch_parse_cpp`,
  `cpp_positive_sema_qualified_type_spelling_shared_parse_cpp`,
  `cpp_positive_sema_qualified_type_start_probe_parse_cpp`, and
  `cpp_positive_sema_qualified_type_start_shared_probe_parse_cpp`
- reran the required clean before/after full suite and passed the monotonic
  regression guard with no new failures:
  `before passed=2272/2273`, `after passed=2272/2273`; the existing
  `verify_tests_verify_top_level_recovery` failure remained unchanged
- tightened `cpp_parser_debug_qualified_type_spelling_stack` to require a
  normalized summary stack with one compact
  `parse_next_template_argument` / `try_parse_template_type_arg` pair instead
  of the duplicated nested wrapper sequence
- normalized parser-debug summary emission so consecutive repeated
  `parse_next_template_argument` / `try_parse_template_type_arg` pairs are
  compacted in the committed summary stack without changing the full debug
  event stream
- reran focused parser-debug coverage for
  `cpp_parser_debug_qualified_type_top_level_params`,
  `cpp_parser_debug_qualified_type_template_arg_stack`,
  `cpp_parser_debug_qualified_type_dependent_typename_stack`,
  `cpp_parser_debug_qualified_type_typename_spelling_stack`, and
  `cpp_parser_debug_qualified_type_spelling_stack`
- reran nearby qualified-type parser coverage for
  `cpp_positive_sema_qualified_dependent_typename_global_parse_cpp`,
  `cpp_positive_sema_qualified_type_resolution_dispatch_parse_cpp`,
  `cpp_positive_sema_qualified_type_spelling_shared_parse_cpp`,
  `cpp_positive_sema_qualified_type_start_probe_parse_cpp`, and
  `cpp_positive_sema_qualified_type_start_shared_probe_parse_cpp`
- reran the required clean before/after full suite and passed the regression
  guard with no new failures or timeout-policy regressions:
  `before passed=2272/2273`, `after passed=2272/2273`; the existing
  `verify_tests_verify_top_level_recovery` failure remained unchanged
- added reduced parser-debug regression coverage in
  `cpp_parser_debug_if_init_qualified_probe_leaf` for an unsupported
  `if (const size_t len = 1)` init-statement where a speculative
  qualified-type probe was correctly preserved in the stack line but the
  committed summary previously degraded to
  `parse_fn=try_parse_qualified_base_type`
- tightened parser-debug leaf selection so when a committed parser failure is
  followed only by speculative qualified-type probe frames in the summary
  stack, `parse_fn=...` stays anchored on the committed owning parser frame
  while the stack line continues to show the deeper probe detail
- reran focused parser-debug coverage for
  `cpp_parser_debug_expr_stmt_stack`,
  `cpp_parser_debug_record_member_stack`,
  `cpp_parser_debug_record_member_param_default_rank`,
  `cpp_parser_debug_record_member_type_like_rank`,
  `cpp_parser_debug_record_member_using_alias_leaf`,
  `cpp_parser_debug_record_member_typedef_leaf`,
  `cpp_parser_debug_qualified_type_top_level_params`,
  `cpp_parser_debug_qualified_type_template_arg_stack`,
  `cpp_parser_debug_qualified_type_dependent_typename_stack`,
  `cpp_parser_debug_qualified_type_typename_spelling_stack`,
  `cpp_parser_debug_qualified_type_spelling_stack`, and
  `cpp_parser_debug_if_init_qualified_probe_leaf`
- reran the required clean before/after full suite and passed the regression
  guard with no new failures:
  `before passed=2272/2273`, `after passed=2274/2275`; the existing
  `verify_tests_verify_top_level_recovery` failure remained unchanged

## Next Intended Slice

- resample `./build/c4cll --parser-debug --parse-only tests/cpp/std/std_vector_simple.cpp`
  and identify the next remaining wrapper-heavy summary that still collapses
  after the new top-level qualified-probe leaf fix
- prefer a candidate that distinguishes best-failure replacement or summary
  stack preservation from plain top-level leaf selection, so the next Step 5
  slice pushes toward the eventual committed-failure vs no-match cleanup

## Blockers

- no blocker on the normalized nested template-argument summary path
- the next slice likely needs explicit tri-state or ranking rules so
  soft-failure bookkeeping can be normalized without hiding the committed
  local cause once a concrete repro is selected
- no blocker on the committed-leaf vs qualified-type-probe summary case; the
  top-level wrapper-only summary case is now covered by a reduced regression
  and a local summary-leaf fix
- the next slice is not yet reduced to a standalone file; the current search
  anchor remains the parser-debug output from
  `tests/cpp/std/std_vector_simple.cpp`

## Resume Notes

- active repro command:
  `./build/c4cll --parser-debug --parse-only tests/cpp/std/std_vector_simple.cpp`
- landed reduced repro for this slice:
  `./build/c4cll --parser-debug --parse-only tests/cpp/internal/negative_case/parser_debug_top_level_qualified_probe_leaf.cpp`
- current reduced repro candidates for the next iteration:
  `./build/c4cll --parser-debug --parse-only tests/cpp/std/std_vector_simple.cpp`
  with the current highest-value diagnostic target still to be selected from
  the remaining wrapper-heavy failures after the new top-level qualified-probe
  leaf fix
- the previously recorded qualified-type candidates
  `parser_debug_qualified_type_typename_spelling_stack.cpp` and
  `parser_debug_qualified_type_spelling_stack.cpp` now look stable on the
  current tree and do not appear to be the next Step 5 regression
- the new top-level wrapper regression now keeps
  `parse_fn=try_parse_qualified_base_type` while preserving the existing
  `parse_top_level -> try_parse_cpp_scoped_base_type ->
  try_parse_qualified_base_type` stack prefix in the emitted summary line
- the next follow-up should still keep the detailed event log untouched unless
  the next reduced repro proves a missing local guard rather than a
  ranking/summary rule
- the repo is not currently building this target as C++20, so `std::source_location`
  was not adopted; the current non-macro path uses `__func__`
- the parked `std::vector` bring-up work remains in
  `ideas/open/std_vector_bringup_plan.md` and should not be silently folded
  back into this diagnostics plan
