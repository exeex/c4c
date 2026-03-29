# Parser Error Diagnostics Todo

Status: Active
Source Idea: ideas/open/parser_error_diagnostics_plan.md
Source Plan: plan.md

## Active Item

- Step 1 / Step 2: normalize the parser trace model around parse function names
  and extend `ParseContextGuard` coverage across significant `parse_*`
  functions without changing parser behavior

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

## Next Intended Slice

- add `ParseContextGuard` to the remaining meaningful `parse_*` entry points,
  then make `--parser-debug` print a stable full parse-function stack snapshot
  for the best failure

## Blockers

- the current debug output still shows sparse stacks because only a subset of
  parser entry points are instrumented
- speculative `try_parse_*` rewinds still collapse some root causes before the
  debug stack can fully explain them

## Resume Notes

- active repro command:
  `./build/c4cll --parser-debug --parse-only tests/cpp/std/std_vector_simple.cpp`
- current prototype prints short default errors and debug events such as
  `fn=parse_param`, but the full parse-function stack still needs broader guard
  coverage to become consistently useful
- the repo is not currently building this target as C++20, so `std::source_location`
  was not adopted; the current non-macro path uses `__func__`
- the parked `std::vector` bring-up work remains in
  `ideas/open/std_vector_bringup_plan.md` and should not be silently folded
  back into this diagnostics plan
