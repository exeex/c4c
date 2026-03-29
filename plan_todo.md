# Parser Error Diagnostics Todo

Status: Active
Source Idea: ideas/open/parser_error_diagnostics_plan.md
Source Plan: plan.md

## Active Item

- Step 3: improve `--parser-debug` output shape so best-failure stacks stay
  readable on recursive/speculative parser paths without changing parser
  behavior
- Current slice: keep the detailed `[pdebug] kind=...` event stream intact, but
  collapse duplicate adjacent frames in the emitted best-failure
  `[pdebug] stack:` line and lock that shape in with a reduced parser-only test

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

## Next Intended Slice

- after this formatting pass, test a speculative parse path where
  `try_parse_record_*` rewinds currently leave the event stream informative but
  the final stack summary still needs better root-cause emphasis
- likely first target: a reduced record-member failure that exercises
  `try_parse_record_member_*` rewinds and confirms the summary stack stays
  bounded

## Blockers

- speculative `try_parse_*` rewinds still collapse some root causes before the
  debug stack can fully explain them
- the debug trace is now broader, but stack readability still degrades on deep
  recursive/speculative paths because every nested frame is printed verbatim

## Resume Notes

- active repro command:
  `./build/c4cll --parser-debug --parse-only tests/cpp/std/std_vector_simple.cpp`
- this iteration is targeting stack readability rather than additional guard
  coverage
- keep the detailed event log untouched; only the summary `[pdebug] stack:`
  line should become less repetitive on nested recursion
- current reduced regression locks in adjacent-frame compaction for nested
  `parse_unary` recursion; the next slice should prove the same readability
  improvement on a speculative record-member parse path
- the repo is not currently building this target as C++20, so `std::source_location`
  was not adopted; the current non-macro path uses `__func__`
- the parked `std::vector` bring-up work remains in
  `ideas/open/std_vector_bringup_plan.md` and should not be silently folded
  back into this diagnostics plan
