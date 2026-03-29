# Parser Error Diagnostics Todo

Status: Active
Source Idea: ideas/open/parser_error_diagnostics_plan.md
Source Plan: plan.md

## Active Item

- Step 5: prepare the next diagnostic slice by bounding the first
  committed-failure vs no-match follow-up under speculative `try_parse_*`
  record-member rewinds
- Current slice: instrument the next qualified-type speculative dispatch that
  still masks the local helper stack under outer declaration/type wrappers
- Iteration target: add a reduced parser-debug repro around
  the next adjacent nested qualified-type probe under
  `parse_next_template_argument` where the emitted summary keeps the committed
  leaf but still drops a local helper frame after leaving
  `try_parse_cpp_scoped_base_type`

## Completed

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

## Next Intended Slice

- move to the next namespace/type speculative dispatch that still masks the
  best inner failure after the nested qualified-type template-argument case
- first candidate after this slice: inspect whether
  `consume_qualified_type_spelling_with_typename` or adjacent qualified-name
  helpers need explicit stack-preservation coverage when the dependent-typename
  leaf is correct but the fully spelled nested qualified-name path is still not
  visible in the summary

## Blockers

- speculative `try_parse_*` rewinds still collapse some root causes into the
  outer committed failure ranking even though the summary stack is now more
  informative
- the event stream remains useful, but the next slice likely needs explicit
  tri-state or ranking rules so soft-failure bookkeeping does not mask the
  committed leaf cause

## Resume Notes

- active repro command:
  `./build/c4cll --parser-debug --parse-only tests/cpp/std/std_vector_simple.cpp`
- current reduced repro candidate for this iteration:
  `./build/c4cll --parser-debug --parse-only <tmp>.cpp` with a nested
  qualified-type template argument that leaves `parse_next_template_argument`
  through `try_parse_cpp_scoped_base_type`, then later fails in a top-level
  wrapper after the helper stack has been ranked away
- this iteration landed the dependent-typename repro as
  `tests/cpp/internal/negative_case/parser_debug_qualified_type_dependent_typename_stack.cpp`
  and moved the emitted summary stack from
  `try_parse_cpp_scoped_base_type -> parse_top_level_parameter_list` to
  `try_parse_cpp_scoped_base_type -> parse_dependent_typename_specifier ->
  parse_top_level_parameter_list`
- this iteration landed the file-scope qualified-type declarator repro as
  `tests/cpp/internal/negative_case/parser_debug_qualified_type_top_level_params.cpp`
  and moved the emitted leaf from `parse_top_level` to
  `parse_top_level_parameter_list`
- the current target is the next qualified-type speculative parse helper after
  the nested qualified-type template-argument slice, starting with
  dependent-typename or adjacent nested qualified-type probes under
  `parse_next_template_argument`
- the current slice kept the qualified-type parameter failure leaf at
  `parse_top_level_parameter_list` but now preserves the preceding
  `try_parse_cpp_scoped_base_type` and `try_parse_qualified_base_type` frames
  in the emitted summary stack
- the current slice also preserves the nested
  `parse_next_template_argument -> try_parse_cpp_scoped_base_type ->
  try_parse_qualified_base_type` path when a later top-level wrapper failure
  becomes the committed root cause
- this iteration completed the alias/typedef leaf-local diagnostics slice
  without changing the broader speculative ranking heuristics yet
- keep the detailed event log untouched; the current summary logic now reuses
  the furthest/deepest recorded stack when `parse_top_level` is only a wrapper
- reduced regressions now cover both nested `parse_unary` recursion and a
  nested record-member rewind path, plus malformed record `using` and `typedef`
  alias members
- the repo is not currently building this target as C++20, so `std::source_location`
  was not adopted; the current non-macro path uses `__func__`
- the parked `std::vector` bring-up work remains in
  `ideas/open/std_vector_bringup_plan.md` and should not be silently folded
  back into this diagnostics plan
