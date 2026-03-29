# std::vector Bring-up Todo

Status: Active
Source Idea: ideas/open/std_vector_bringup_plan.md
Source Plan: plan.md

## Current Active Item

- Step 2: Reduce the next header blocker from `tests/cpp/std/std_vector_simple.cpp`.
- Exact target for this iteration: fix the reduced free-function parameter parsing blocker where prior C++ class/struct tags are not recognized as type names, leaving `&` / `&&` behind in declarations like `void f(E&);`.

## Completed Items

- Activated `ideas/open/std_vector_bringup_plan.md` into `plan.md`.
- Created `plan_todo.md` for the active runbook.
- Captured a fresh baseline in `test_before.log`; the only baseline full-suite failure was `verify_tests_verify_top_level_recovery`.
- Reduced the `stl_algobase.h` `if (const size_t __len = ...)` parser failure to `tests/cpp/internal/postive_case/if_condition_decl_parse.cpp`.
- Implemented C++ `if` declaration-condition parsing in `src/frontend/parser/statements.cpp` by lowering it to a scoped synthetic block plus ordinary `NK_IF`.
- Tightened the declaration-condition probe so comparison expressions like `if (!(b > a))` do not get misclassified as declaration conditions.
- Retired stale negative/parser-debug fixtures that only existed to pin the old unsupported `if`-declaration failure mode.
- Re-ran the full suite from a clean build into `test_after.log`; the after-run failure set returned to the same single baseline failure `verify_tests_verify_top_level_recovery`.
- Reproduced the current `std_vector_simple.cpp` frontier and confirmed earlier stable blockers now include:
  - `/usr/include/c++/14/bits/exception.h:65:30` `expected=RPAREN got='&'`
  - `/usr/include/c++/14/bits/exception.h:67:24` `expected=RPAREN got='&&'`
- Reduced that header failure to a standalone free-function repro: prior `struct E;` declarations are not treated as C++ type names in parameter lists, so `void f(E&);` and `void f(const E&&);` misparse.
- Added `tests/cpp/internal/postive_case/free_function_record_ref_param_parse.cpp` and retired the stale negative parser-debug fixture that only pinned the old unsupported failure mode.
- Fixed C++ record-tag injection for forward declarations and namespace-local class-name lookup so free-function `E&` / `E&&` parameters parse while existing namespace struct runtime coverage continues to pass.
- Re-ran targeted parser/runtime coverage and a clean full-suite `ctest`; the suite returned to the same single baseline failure `verify_tests_verify_top_level_recovery`.

## Next Intended Slice

- Reproduce `./build/c4cll --parse-only tests/cpp/std/std_vector_simple.cpp` again and reduce the earliest blocker that still survives after this class-name injection fix.
- Treat `bits/exception.h` as likely downstream noise unless an isolated reduced case still fails; the next true frontier is probably one of:
  - `/usr/include/c++/14/bits/stl_iterator.h:1494:15` `typedef uses unknown base type: _Iterator`
  - `/usr/include/c++/14/bits/predefined_ops.h:150:32` `expected=RPAREN got='__comp'`
  - `/usr/include/c++/14/bits/new_allocator.h:70:15` `typedef uses unknown base type: _Tp`
- Add one reduced regression for that surviving blocker before changing compiler behavior.

## Blockers

- No blocking infrastructure issue remains.
- Note: the monotonic regression script reports a lower passed-test count only because this slice intentionally removed one stale negative parser-debug test; the clean full-suite failing test set itself remained unchanged at the single baseline failure `verify_tests_verify_top_level_recovery`.

## Resume Notes

- The repo had no active `plan.md` or `plan_todo.md`; this iteration repaired planning state by activating the only open idea.
- `tests/cpp/internal/postive_case/if_condition_decl_parse.cpp` is the committed reduced repro for the fixed `if` declaration-condition syntax.
- The free-function class-name reference gap is now covered by `free_function_record_ref_param_parse.cpp` and no longer introduces namespace-struct regressions.
- `std_vector_simple.cpp` still reports the same broad frontier, so this slice improved parser coverage without yet advancing the main libstdc++ repro; re-reduce the earliest surviving blocker before the next implementation change.
