# Parser Error Diagnostics Todo

Status: Active
Source Idea: ideas/open/parser_error_diagnostics_plan.md
Source Plan: plan.md

## Active Item

- Step 5: prepare the next diagnostic slice by bounding the first
  committed-failure vs no-match follow-up under speculative `try_parse_*`
  record-member rewinds
- Current slice: keep `try_parse_record_type_like_member_dispatch` visible in
  parser-debug summaries when a malformed type-like member target would
  otherwise collapse to the outer record-member wrapper

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

## Next Intended Slice

- decide whether the next speculative slice should stay on type-like members by
  surfacing a deeper committed token-level cause inside alias/typedef parsing,
  or move to the next namespace/type speculative dispatch that still masks the
  best inner failure

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
- this iteration delivered the type-like record-member dispatch stack/leaf
  preservation, not a deeper token-level committed-cause fix inside alias or
  typedef parsing
- next iteration should decide whether to make malformed type-like alias targets
  record a deeper committed failure, or leave this family parked and move to the
  next speculative dispatch that still collapses to a wrapper-only root cause
- this iteration stayed on targeted instrumentation/coverage rather than
  changing broader speculative ranking rules
- keep the detailed event log untouched; the current summary logic now reuses
  the furthest/deepest recorded stack when `parse_top_level` is only a wrapper
- reduced regressions now cover both nested `parse_unary` recursion and a
  nested record-member rewind path
- the repo is not currently building this target as C++20, so `std::source_location`
  was not adopted; the current non-macro path uses `__func__`
- the parked `std::vector` bring-up work remains in
  `ideas/open/std_vector_bringup_plan.md` and should not be silently folded
  back into this diagnostics plan
