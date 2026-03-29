# std::vector Bring-up Todo

Status: Active
Source Idea: ideas/open/std_vector_bringup_plan.md
Source Plan: plan.md

## Current Active Item

- Step 2: Reduce the next header blocker from `tests/cpp/std/std_vector_simple.cpp`.
- Exact target for this iteration: reduce the first parser blocker now exposed after the `bits/stl_iterator.h` branch-selection fix, starting with the earliest C++20/concepts parse failures from `/usr/include/c++/14/type_traits` and `/usr/include/c++/14/bits/iterator_concepts.h`.

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
- Added `tests/cpp/internal/postive_case/cpp20_feature_macro_branch_parse.cpp` to lock in libstdc++-style `#if __cplusplus > 201703L && __cpp_concepts >= 201907L && __cpp_constexpr >= 201811L` branch selection.
- Updated C++ predefined macros in `src/frontend/preprocessor/preprocessor.cpp` so the C++ source profile now advertises `__cplusplus=202002L`, `__cpp_constexpr=201811L`, and `__cpp_concepts=201907L`.
- Verified the new reduced test passes and that `./build/c4cll --pp-only /tmp/include_stl_iterator.cpp` now selects the modern `move_iterator` branch instead of emitting the old `typedef _Iterator pointer;` fallback.
- Re-ran the direct header and `std_vector_simple.cpp` repros; both advanced past `bits/stl_iterator.h:1494:15` and now first fail in C++20/concepts-heavy headers, starting with:
  - `/usr/include/c++/14/type_traits:3442:13` `expected=RPAREN got='!'`
  - `/usr/include/c++/14/bits/iterator_concepts.h:184:15` `expected=RPAREN got='!'`
  - `/usr/include/c++/14/bits/iterator_concepts.h:267:12` `expected template parameter name`
- Re-ran the full suite into `test_after.log`; `verify_tests_verify_top_level_recovery` now passes, and the current remaining suite failures are seven backend LIR variadic ABI tests:
  - `backend_lir_aarch64_variadic_dpair_ir`
  - `backend_lir_aarch64_variadic_float_array_ir`
  - `backend_lir_aarch64_variadic_nested_float_array_ir`
  - `backend_lir_aarch64_variadic_float4_ir`
  - `backend_lir_aarch64_variadic_double4_ir`
  - `backend_lir_aarch64_variadic_single_double_ir`
  - `backend_lir_aarch64_variadic_single_float_ir`

## Next Intended Slice

- Keep the focus on the first exposed C++20/concepts parser frontier rather than the old `move_iterator` fallback branch.
- Reduce the earliest `!`-based parameter-list failure from `/usr/include/c++/14/type_traits` or `/usr/include/c++/14/bits/iterator_concepts.h` into the smallest standalone parse test.
- Land the smallest parser fix for that reduced concepts-era syntax, then re-run:
  - `./build/c4cll --parse-only /tmp/include_stl_iterator.cpp`
  - `./build/c4cll --parse-only tests/cpp/std/std_vector_simple.cpp`
  - targeted C++ parse tests before any new full-suite pass.

## Blockers

- No blocking infrastructure issue remains.
- Current technical blocker: after correcting the preprocessor branch selection, the true first frontier moved into unsupported C++20/concepts parsing in `/usr/include/c++/14/type_traits` and `/usr/include/c++/14/bits/iterator_concepts.h`; the next safe slice is to reduce and fix the earliest one of those parser failures.

## Resume Notes

- The repo had no active `plan.md` or `plan_todo.md`; this iteration repaired planning state by activating the only open idea.
- `tests/cpp/internal/postive_case/if_condition_decl_parse.cpp` is the committed reduced repro for the fixed `if` declaration-condition syntax.
- The free-function class-name reference gap is now covered by `free_function_record_ref_param_parse.cpp` and no longer introduces namespace-struct regressions.
- `std_vector_simple.cpp` no longer stops first on `bits/stl_iterator.h:1494:15`; the first current blockers are now C++20/concepts parser gaps in `type_traits` and `iterator_concepts`.
- The new reduced coverage is `tests/cpp/internal/postive_case/cpp20_feature_macro_branch_parse.cpp`, which should stay green while the parser catches up to the newly exposed modern-header syntax.
