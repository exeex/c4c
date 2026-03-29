# std::vector Bring-up Todo

Status: Active
Source Idea: ideas/open/std_vector_bringup_plan.md
Source Plan: plan.md

## Current Active Item

- Step 2: Reduce the next header blocker from `tests/cpp/std/std_vector_simple.cpp`.
- Exact target for the next iteration: reduce one of the new post-`if`-declaration frontier failures now visible under `bits/stl_bvector.h`, `bits/stl_function.h`, or `bits/vector.tcc`.

## Completed Items

- Activated `ideas/open/std_vector_bringup_plan.md` into `plan.md`.
- Created `plan_todo.md` for the active runbook.
- Captured a fresh baseline in `test_before.log`; the only baseline full-suite failure was `verify_tests_verify_top_level_recovery`.
- Reduced the `stl_algobase.h` `if (const size_t __len = ...)` parser failure to `tests/cpp/internal/postive_case/if_condition_decl_parse.cpp`.
- Implemented C++ `if` declaration-condition parsing in `src/frontend/parser/statements.cpp` by lowering it to a scoped synthetic block plus ordinary `NK_IF`.
- Tightened the declaration-condition probe so comparison expressions like `if (!(b > a))` do not get misclassified as declaration conditions.
- Retired stale negative/parser-debug fixtures that only existed to pin the old unsupported `if`-declaration failure mode.
- Re-ran the full suite from a clean build into `test_after.log`; the after-run failure set returned to the same single baseline failure `verify_tests_verify_top_level_recovery`.

## Next Intended Slice

- Reproduce `./build/c4cll --parse-only tests/cpp/std/std_vector_simple.cpp` and reduce the earliest remaining stable blocker from the new frontier:
  - `/usr/include/c++/14/bits/stl_bvector.h:1570:27` `expected=RPAREN got='*'`
  - `/usr/include/c++/14/bits/stl_bvector.h:1612:36` `expected=RPAREN got='.'`
  - `/usr/include/c++/14/bits/stl_function.h:123:15` `typedef uses unknown base type: _Result`
  - `/usr/include/c++/14/bits/vector.tcc:80:24` `expected=RPAREN got='->'`
- Add one reduced regression test for the chosen blocker before changing compiler behavior.

## Blockers

- No blocking infrastructure issue remains.
- Note: `test_after.log` has fewer total tests than `test_before.log` because this slice intentionally removed stale negative fixtures that encoded the old unsupported parser behavior; the full-suite failing test set itself remained unchanged.

## Resume Notes

- The repo had no active `plan.md` or `plan_todo.md`; this iteration repaired planning state by activating the only open idea.
- `tests/cpp/internal/postive_case/if_condition_decl_parse.cpp` is the committed reduced repro for the fixed `if` declaration-condition syntax.
- `std_vector_simple.cpp` no longer stops on the earlier `stl_algobase.h` `const` declaration-condition failures; the frontier moved deeper into later libstdc++ headers.
