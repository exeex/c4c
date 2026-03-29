# Parser Error Diagnostics Todo

Status: Active
Source Idea: ideas/open/parser_error_diagnostics_plan.md
Source Plan: plan.md

## Active Item

- Step 1 / Step 2: normalize the parser trace model around parse function names
  and extend `ParseContextGuard` coverage across significant `parse_*`
  functions without changing parser behavior
- Current slice: add guards to the remaining high-signal statement,
  declaration, and expression entry points, then add a reduced
  `--parser-debug` parser-only test that asserts the emitted stack shape

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

## Next Intended Slice

- after this coverage pass, tighten `--parser-debug` formatting so the best
  failure stack is easier to scan when speculative `try_parse_*` rewinds are
  involved
- likely first target: reduce duplicate/noisy stack frames from nested
  recursive expression parsing and the `try_parse_record_*` speculative paths

## Blockers

- speculative `try_parse_*` rewinds still collapse some root causes before the
  debug stack can fully explain them
- the debug trace is now broader, but stack readability still degrades on deep
  recursive/speculative paths because every nested frame is printed verbatim

## Resume Notes

- active repro command:
  `./build/c4cll --parser-debug --parse-only tests/cpp/std/std_vector_simple.cpp`
- this iteration is targeting the non-trivial entry surface first:
  `parse_stmt`, `parse_local_decl`, `parse_top_level`, `parse_assign_expr`,
  `parse_ternary`, `parse_unary`, and nearby wrappers where stack depth is
  currently lost
- current prototype prints short default errors and debug events such as
  `fn=parse_param`; after this slice, reduced expression-statement failures also
  keep a stable `[pdebug] stack:` line in parser-debug mode
- the repo is not currently building this target as C++20, so `std::source_location`
  was not adopted; the current non-macro path uses `__func__`
- the parked `std::vector` bring-up work remains in
  `ideas/open/std_vector_bringup_plan.md` and should not be silently folded
  back into this diagnostics plan
