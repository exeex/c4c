# std::vector Bring-up Todo

Status: Active
Source Idea: ideas/open/std_vector_bringup_plan.md
Source Plan: plan.md

## Current Active Item

- Step 2: Reduce the next header blocker from `tests/cpp/std/std_vector_simple.cpp`.
- Exact target for this iteration: fix the `bits/stl_iterator.h` reduced parse blocker where `move_iterator` member typedefs like `typedef _Iterator pointer;` can lose template-parameter type resolution and fail with `typedef uses unknown base type`.

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
- Reproduced the next surviving blocker directly with `./build/c4cll --parse-only /tmp/include_stl_iterator.cpp` where `#include <bits/stl_iterator.h>` alone still fails at `/usr/include/c++/14/bits/stl_iterator.h:1494:15` with `typedef uses unknown base type: _Iterator`.
- Reduced the current frontier to `bits/stl_iterator.h` itself and verified on scratch copies that deleting the `move_iterator` line `using __base_ref = typename __traits_type::reference;` makes the header parse, while deleting nearby `friend`, `iterator_type`, `value_type`, or `difference_type` lines does not.
- Verified that this iteration did not yet yield a safe compiler fix: speculative parser patches and a direct regression test for `bits/stl_iterator.h` were reverted after the header failure remained unchanged.

## Next Intended Slice

- Keep the focus on `bits/stl_iterator.h:1494:15`.
- Instrument or reduce the `move_iterator` member-declaration path around:
  - `using __traits_type = iterator_traits<_Iterator>;`
  - `using __base_ref = typename __traits_type::reference;`
  - the subsequent `typedef _Iterator pointer;`
- Land the smallest parser fix that preserves `move_iterator` class-body state through that sequence, then re-run:
  - `./build/c4cll --parse-only /tmp/include_stl_iterator.cpp`
  - `./build/c4cll --parse-only tests/cpp/std/std_vector_simple.cpp`
  - targeted C++ parse tests before any new full-suite pass.

## Blockers

- No blocking infrastructure issue remains.
- Current technical blocker: the true first frontier is still `bits/stl_iterator.h`, but the failure appears to come from class-member parse state around the dependent alias `using __base_ref = typename __traits_type::reference;`; the exact parser routine still needs a smaller safe fix.

## Resume Notes

- The repo had no active `plan.md` or `plan_todo.md`; this iteration repaired planning state by activating the only open idea.
- `tests/cpp/internal/postive_case/if_condition_decl_parse.cpp` is the committed reduced repro for the fixed `if` declaration-condition syntax.
- The free-function class-name reference gap is now covered by `free_function_record_ref_param_parse.cpp` and no longer introduces namespace-struct regressions.
- `std_vector_simple.cpp` still reports the same broad frontier, and the first real blocker remains `bits/stl_iterator.h`.
- A scratch-header experiment showed that removing only `using __base_ref = typename __traits_type::reference;` lets `#include <bits/stl_iterator.h>` parse, so the next slice should stay tightly centered on the `move_iterator` dependent member-alias sequence instead of jumping ahead to `predefined_ops.h` or `new_allocator.h`.
