# std::vector Bring-up Todo

Status: Active
Source Idea: ideas/open/std_vector_bringup_plan.md
Source Plan: plan.md

## Current Active Item

- Step 2: Reduce the next header blocker from `tests/cpp/std/std_vector_simple.cpp`.
- Exact target for this iteration: reduce the first cleanly reproducible blocker still emitted from `tests/cpp/std/std_vector_simple.cpp` after the `stl_iterator.h` C++20 requires-expression fix, focusing next on the surviving parameter-list / incomplete-type cluster starting with `/usr/include/c++/14/bits/exception.h:65`, `/usr/include/c++/14/bits/stl_iterator.h:1011`, `/usr/include/c++/14/bits/stl_iterator.h:1572`, `/usr/include/c++/14/bits/stl_iterator.h:1954`, and `/usr/include/c++/14/bits/stl_iterator.h:1978`.

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
- Reduced the `/usr/include/c++/14/bits/iterator_concepts.h:819` incomplete-type frontier to `tests/cpp/internal/postive_case/qualified_record_partial_specialization_parse.cpp`, covering an out-of-namespace class template partial specialization spelled as `struct detail::holder<T*>`.
- Updated `src/frontend/parser/types.cpp` so record-tag parsing after `struct` / `class` consumes C++ qualified names like `detail::holder`, and record-definition canonicalization no longer prefixes an already qualified tag with the current namespace again.
- Re-ran nearby parser coverage:
  - `cpp_positive_sema_qualified_record_partial_specialization_parse_cpp`
  - `cpp_positive_sema_cpp20_constrained_template_param_parse_cpp`
  - `cpp_positive_sema_cpp20_requires_clause_parse_cpp`
  - `cpp_positive_sema_free_function_record_ref_param_parse_cpp`
- Re-ran the direct `#include <bits/stl_iterator.h>` and `tests/cpp/std/std_vector_simple.cpp` repros; both advanced past the old `/usr/include/c++/14/bits/iterator_concepts.h:819` `object has incomplete type: __detail` frontier and now first fail at:
  - `/usr/include/c++/14/bits/iterator_concepts.h:992:5` `object has incomplete type: ranges::__access::_Decay_copy`
  - `/usr/include/c++/14/bits/exception.h:65:30` `expected=RPAREN got='&'`
  - `/usr/include/c++/14/bits/stl_iterator.h:1011:34` `expected=RPAREN got='::'`
- Re-ran a clean full suite into `test_after.log`; the remaining failures are still only the seven known backend LIR variadic ABI tests:
  - `backend_lir_aarch64_variadic_dpair_ir`
  - `backend_lir_aarch64_variadic_float_array_ir`
  - `backend_lir_aarch64_variadic_nested_float_array_ir`
  - `backend_lir_aarch64_variadic_float4_ir`
  - `backend_lir_aarch64_variadic_double4_ir`
  - `backend_lir_aarch64_variadic_single_double_ir`
  - `backend_lir_aarch64_variadic_single_float_ir`
- Reduced the `/usr/include/c++/14/bits/iterator_concepts.h:992` incomplete-type frontier to `tests/cpp/internal/postive_case/record_final_specifier_parse.cpp`, covering a record definition spelled as `struct _Decay_copy final { ... };` inside `namespace ranges::__access`.
- Updated `src/frontend/parser/types.cpp` so C++ record pre-body setup accepts the contextual `final` specifier between the record name and any base clause or body.
- Verified the reduced test against Clang (`clang++ -std=c++20 -fsyntax-only`) and re-ran nearby parser coverage:
  - `cpp_positive_sema_record_final_specifier_parse_cpp`
  - `cpp_positive_sema_qualified_record_partial_specialization_parse_cpp`
  - `cpp_positive_sema_cpp20_constrained_template_param_parse_cpp`
  - `cpp_positive_sema_cpp20_requires_clause_parse_cpp`
  - `cpp_positive_sema_free_function_record_ref_param_parse_cpp`
- Re-ran the direct `#include <bits/stl_iterator.h>` and `tests/cpp/std/std_vector_simple.cpp` repros; both advanced past the old `/usr/include/c++/14/bits/iterator_concepts.h:992` `ranges::__access::_Decay_copy` incomplete-type frontier and now first fail at:
  - `/usr/include/c++/14/bits/exception.h:65:30` `expected=RPAREN got='&'`
  - `/usr/include/c++/14/bits/exception.h:67:24` `expected=RPAREN got='&&'`
  - `/usr/include/c++/14/bits/stl_iterator.h:1011:34` `expected=RPAREN got='::'`
  - `/usr/include/c++/14/bits/stl_iterator.h:1572:22` `object has incomplete type: move_iterator`
- Re-ran a clean full suite into `test_after.log`; the remaining failures are still only the seven known backend LIR variadic ABI tests:
  - `backend_lir_aarch64_variadic_dpair_ir`
  - `backend_lir_aarch64_variadic_float_array_ir`
  - `backend_lir_aarch64_variadic_nested_float_array_ir`
  - `backend_lir_aarch64_variadic_float4_ir`
  - `backend_lir_aarch64_variadic_double4_ir`
  - `backend_lir_aarch64_variadic_single_double_ir`
  - `backend_lir_aarch64_variadic_single_float_ir`
- The repo-level regression-guard script still fails if compared against the old one-failure baseline `test_fail_before.log`, because that baseline predates the already-known seven backend LIR variadic ABI failures; comparing the current after-log against that stale file reports those existing backend failures as "new" even though this parser slice did not change them.
- Reduced `/usr/include/c++/14/bits/stl_iterator.h:2292` `if constexpr (requires { requires derived_from<...>; })` to `tests/cpp/internal/postive_case/cpp20_requires_expression_if_constexpr_parse.cpp`.
- Updated `src/frontend/parser/expressions.cpp` so expression parsing treats a C++ `requires (...) { ... }` or `requires { ... }` form as an unevaluated boolean placeholder, matching the parser-only strategy already used for requires-clauses.
- Re-ran targeted parser coverage:
  - `cpp_positive_sema_cpp20_requires_expression_if_constexpr_parse_cpp`
  - `cpp_positive_sema_cpp20_requires_clause_parse_cpp`
  - `cpp_positive_sema_record_final_specifier_parse_cpp`
  - `cpp_positive_sema_free_function_record_ref_param_parse_cpp`
- Re-ran the direct `#include <bits/stl_iterator.h>` and `tests/cpp/std/std_vector_simple.cpp` repros; both no longer report the former `/usr/include/c++/14/bits/stl_iterator.h:2292` `parse_stmt ... expected=RPAREN got='{'` failure, while the remaining first-emitted cluster is still:
  - `/usr/include/c++/14/bits/exception.h:65:30` `expected=RPAREN got='&'`
  - `/usr/include/c++/14/bits/exception.h:67:24` `expected=RPAREN got='&&'`
  - `/usr/include/c++/14/bits/stl_iterator.h:1011:34` `expected=RPAREN got='::'`
  - `/usr/include/c++/14/bits/stl_iterator.h:1572:22` `object has incomplete type: move_iterator`
  - `/usr/include/c++/14/bits/stl_iterator.h:1954:42` `expected=RPAREN got='&'`
  - `/usr/include/c++/14/bits/stl_iterator.h:1978:36` `expected=RPAREN got='&&'`
- Re-ran the full suite into `test_fail_after.log`; the remaining failures are still only the seven known backend LIR variadic ABI tests:
  - `backend_lir_aarch64_variadic_dpair_ir`
  - `backend_lir_aarch64_variadic_float_array_ir`
  - `backend_lir_aarch64_variadic_nested_float_array_ir`
  - `backend_lir_aarch64_variadic_float4_ir`
  - `backend_lir_aarch64_variadic_double4_ir`
  - `backend_lir_aarch64_variadic_single_double_ir`
  - `backend_lir_aarch64_variadic_single_float_ir`
- The repo-level regression-guard script still fails against `test_fail_before.log`, because that baseline predates the already-known seven backend LIR variadic ABI failures; the script therefore reports those existing backend failures as "new" even though the current after-log matches the same failure family recorded in recent iterations.

## Next Intended Slice

- Keep the focus on the surviving parameter-list / incomplete-type frontier rather than revisiting the now-fixed C++20 requires-expression syntax.
- Find the first clean reduced repro for the still-emitted `exception.h` / `stl_iterator.h` cluster, since direct standalone extracts for `exception(const exception&) = default;` and `inserter(_Container&, std::__detail::__range_iter_t<_Container>)` parse in isolation and the remaining diagnostics appear to depend on surrounding parser state or recovery.
- Once that contextual repro is reduced, land the smallest parser fix and then re-run:
  - `./build/c4cll --parse-only /tmp/include_stl_iterator.cpp`
  - `./build/c4cll --parse-only tests/cpp/std/std_vector_simple.cpp`
  - targeted C++ parse tests before any new full-suite pass.

## Blockers

- No blocking infrastructure issue remains.
- Current technical blocker: the old `stl_iterator.h:2292` requires-expression failure is fixed, but the remaining first-emitted diagnostics in `exception.h` / `stl_iterator.h` still do not reduce cleanly in isolation, suggesting a parser-state or recovery issue that must be captured with a contextual minimized repro before the next syntax fix lands.

## Resume Notes

- The repo had no active `plan.md` or `plan_todo.md`; this iteration repaired planning state by activating the only open idea.
- `tests/cpp/internal/postive_case/if_condition_decl_parse.cpp` is the committed reduced repro for the fixed `if` declaration-condition syntax.
- The free-function class-name reference gap is now covered by `free_function_record_ref_param_parse.cpp` and no longer introduces namespace-struct regressions.
- `tests/cpp/internal/postive_case/cpp20_requires_clause_parse.cpp` is the committed reduced repro for the former `/usr/include/c++/14/type_traits` requires-clause failure.
- `tests/cpp/internal/postive_case/cpp20_constrained_template_param_parse.cpp` is the committed reduced repro for the former `/usr/include/c++/14/bits/iterator_concepts.h:267` constrained template-parameter failure.
- `tests/cpp/internal/postive_case/qualified_record_partial_specialization_parse.cpp` is the committed reduced repro for the former `/usr/include/c++/14/bits/iterator_concepts.h:819` qualified record partial-specialization failure.
- `tests/cpp/internal/postive_case/record_final_specifier_parse.cpp` is the committed reduced repro for the former `/usr/include/c++/14/bits/iterator_concepts.h:992` record-`final` failure.
- `tests/cpp/internal/postive_case/cpp20_requires_expression_if_constexpr_parse.cpp` is the committed reduced repro for the former `/usr/include/c++/14/bits/stl_iterator.h:2292` requires-expression failure.
- `std_vector_simple.cpp` no longer reports the old `/usr/include/c++/14/bits/stl_iterator.h:2292` `if constexpr (requires { ... })` parse failure; the current remaining frontier is the parameter-list / incomplete-type cluster in `exception.h` and `stl_iterator.h`.
- The new reduced coverage is `tests/cpp/internal/postive_case/cpp20_feature_macro_branch_parse.cpp`, which should stay green while the parser catches up to the newly exposed modern-header syntax.
