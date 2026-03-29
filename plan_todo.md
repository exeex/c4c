# std::vector Bring-up Todo

Status: Active
Source Idea: ideas/open/std_vector_bringup_plan.md
Source Plan: plan.md

## Current Active Item

- Step 2: Reduce the next header blocker from `tests/cpp/std/std_vector_simple.cpp`.
- Exact target for this iteration: reduce the first surviving post-constrained-template-parameter blocker now exposed in `/usr/include/c++/14/bits/iterator_concepts.h`, currently the incomplete-type failure at line 819 after the constrained parameter parse gap was removed.

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
- Reduced the earliest surviving C++20/concepts parser failure from `/usr/include/c++/14/type_traits:3450` to `tests/cpp/internal/postive_case/cpp20_requires_clause_parse.cpp`, covering a templated declaration requires-clause with both `(!trait<T>::value)` and a nested `requires (T& t) { ... }` expression.
- Updated `src/frontend/parser/declarations.cpp` to skip an optional C++20 requires-clause between `template<...>` and the following declaration, allowing the reduced repro to parse without teaching semantics for constraints yet.
- Re-ran the reduced parse test plus nearby parser coverage (`cpp20_feature_macro_branch_parse`, `free_function_record_ref_param_parse`) and confirmed they stay green.
- Re-ran the direct `#include <bits/stl_iterator.h>` and `tests/cpp/std/std_vector_simple.cpp` repros; both advanced past the old `/usr/include/c++/14/type_traits:3442` `expected=RPAREN got='!'` frontier and now fail first in `/usr/include/c++/14/bits/iterator_concepts.h` on `expected template parameter name` around lines 267, 272, 279, and 289.
- Re-ran the full suite into `test_after.log`; the remaining failure set still matches the previously recorded seven backend LIR variadic ABI tests, with no newly failing tests observed in this slice.
- Reduced the first surviving `/usr/include/c++/14/bits/iterator_concepts.h:267` constrained template-parameter failure to `tests/cpp/internal/postive_case/cpp20_constrained_template_param_parse.cpp`, using a qualified concept-id spelling `template<detail::Always T>`.
- Updated `src/frontend/parser/declarations.cpp` so `template<Constraint T>` and `template<ns::Constraint T = ...>` are accepted as type template parameters after the parser rules out builtin and typedef NTTP heads.
- Verified the reduced constrained-parameter parse test against Clang (`clang++ -std=c++20 -fsyntax-only`) and re-ran nearby parser coverage:
  - `cpp_positive_sema_cpp20_constrained_template_param_parse_cpp`
  - `cpp_positive_sema_cpp20_requires_clause_parse_cpp`
  - `cpp_positive_sema_free_function_record_ref_param_parse_cpp`
- Re-ran the direct `#include <bits/stl_iterator.h>` and `tests/cpp/std/std_vector_simple.cpp` repros; both advanced past the `iterator_concepts.h` `expected template parameter name` frontier and now first fail on later incomplete-type / parameter-list issues, starting with:
  - `/usr/include/c++/14/bits/iterator_concepts.h:819:38` `object has incomplete type: __detail`
  - `/usr/include/c++/14/bits/iterator_concepts.h:992:5` `object has incomplete type: ranges::__access::_Decay_copy`
  - `/usr/include/c++/14/bits/exception.h:65:30` `expected=RPAREN got='&'`
- Captured a clean-suite before/after comparison for this slice:
  - `test_before.log` had the seven known backend LIR variadic ABI failures plus the new reduced test before the parser fix.
  - `test_after.log` returned to only the same seven known backend LIR variadic ABI failures, so the pass count improved monotonically and no previously passing test regressed.

## Next Intended Slice

- Keep the focus on the first newly exposed `iterator_concepts` incomplete-type frontier rather than returning to the already-fixed constrained template-parameter syntax.
- Reduce `/usr/include/c++/14/bits/iterator_concepts.h:819` into the smallest standalone parse test, likely around nested `__detail` lookup or dependent type formation inside the projected-iterator support path.
- Land the smallest parser / type-resolution fix for that reduced case, then re-run:
  - `./build/c4cll --parse-only /tmp/include_stl_iterator.cpp`
  - `./build/c4cll --parse-only tests/cpp/std/std_vector_simple.cpp`
  - targeted C++ parse tests before any new full-suite pass.

## Blockers

- No blocking infrastructure issue remains.
- Current technical blocker: after adding constrained template-parameter parsing, the true first frontier moved into later C++20 iterator-concepts support, starting with incomplete-type formation in `/usr/include/c++/14/bits/iterator_concepts.h` around line 819.

## Resume Notes

- The repo had no active `plan.md` or `plan_todo.md`; this iteration repaired planning state by activating the only open idea.
- `tests/cpp/internal/postive_case/if_condition_decl_parse.cpp` is the committed reduced repro for the fixed `if` declaration-condition syntax.
- The free-function class-name reference gap is now covered by `free_function_record_ref_param_parse.cpp` and no longer introduces namespace-struct regressions.
- `tests/cpp/internal/postive_case/cpp20_requires_clause_parse.cpp` is the committed reduced repro for the former `/usr/include/c++/14/type_traits` requires-clause failure.
- `tests/cpp/internal/postive_case/cpp20_constrained_template_param_parse.cpp` is the committed reduced repro for the former `/usr/include/c++/14/bits/iterator_concepts.h:267` constrained template-parameter failure.
- `std_vector_simple.cpp` no longer stops first on the `iterator_concepts` constrained-parameter frontier; the first current blockers are now incomplete-type and later parameter-list issues starting at `/usr/include/c++/14/bits/iterator_concepts.h:819`.
- The new reduced coverage is `tests/cpp/internal/postive_case/cpp20_feature_macro_branch_parse.cpp`, which should stay green while the parser catches up to the newly exposed modern-header syntax.
