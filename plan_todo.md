# Active Todo

Status: Active
Source Idea: ideas/open/04_std_vector_bringup_plan.md
Source Plan: plan.md

## Current Active Item

- Step 4: revalidate the post-memoryfwd frontier, starting with the surviving
  comma-expression parser failures in `/usr/include/c++/14/bits/stl_uninitialized.h`
  ahead of the later attribute, typedef-base, and vector.tcc parameter-list
  frontiers

## Next Intended Slice

- inspect `/usr/include/c++/14/bits/stl_uninitialized.h:179` to reduce the
  first surviving comma-expression failure into a standalone parser regression
- keep the next fix limited to the first real expression/parser root cause
  ahead of the later `[` attribute, typedef-base, and vector.tcc frontiers
- rerun `./build/c4cll --parse-only tests/cpp/std/std_vector_simple.cpp` after
  that fix to confirm the frontier advances beyond the current
  `stl_uninitialized.h` comma-expression failure

## Incomplete Items

- [ ] Step 1: lock the current repro harness
- [ ] Step 2: reduce the next blocker into a minimal failing test
- [ ] Step 3: implement the narrow fix for that reduced test
- [ ] Step 4: rerun the `std::vector` repro and record the new frontier
- [ ] Step 5: run targeted plus full-suite regression checks
- [ ] Step 6: carry the case to a stable passing state

## Completed Items

- [x] Activated `ideas/open/04_std_vector_bringup_plan.md` into `plan.md`
- [x] Created `plan_todo.md` aligned to the same source idea
- [x] Step 1: locked the repro harness by re-running
  `./build/c4cll --parse-only tests/cpp/std/std_vector_simple.cpp` and
  confirming the first actionable frontier is `noexcept(expr)` in
  `/usr/include/c++/14/type_traits`
- [x] Reduced the first blocker to
  `tests/cpp/internal/postive_case/noexcept_expr_parse.cpp` using
  `return noexcept(foo());`, and confirmed it failed before implementation with
  the same `parse_primary` / unexpected `noexcept` shape
- [x] Implemented parser support for `noexcept(expr)` in unary-expression
  context and re-ran the targeted parse tests:
  `keyword_noexcept_nullptr_parse`, `noexcept_decl_parse`,
  `noexcept_expr_parse`, and `noexcept_method_parse`
- [x] Advanced the direct `std::vector` parse-only repro past all leading
  `/usr/include/c++/14/type_traits` `noexcept` failures; the next frontier is
  now in `/usr/include/c++/14/bits/iterator_concepts.h` starting at line 75
- [x] Full-suite regression guard passed with monotonic improvement:
  `2384 -> 2385` passing tests, `0 -> 0` failing tests, and no newly failing
  cases
- [x] Reduced the next `iterator_concepts.h` blocker to
  `tests/cpp/internal/postive_case/cpp20_requires_clause_struct_decl_parse.cpp`
  for `template<typename T> requires is_object_v<T> struct trait;`, and
  confirmed it failed before implementation with the same unexpected `struct`
  in expression shape
- [x] Tightened declaration-level C++20 `requires` clause parsing so template
  constraints stop before a following record declaration, and re-ran the
  targeted parse tests:
  `cpp20_requires_clause_parse` and
  `cpp20_requires_clause_struct_decl_parse`
- [x] Advanced the direct `std::vector` parse-only repro beyond the
  `iterator_concepts.h` `struct` frontier; the next failures now start in
  `/usr/include/c++/14/bits/stl_iterator.h:1011` (`::` in a parameter type),
  followed by later parameter-list and incomplete-type issues in iterator and
  allocator headers
- [x] Full-suite regression guard passed with monotonic improvement:
  `2385 -> 2386` passing tests, `0 -> 0` failing tests, and no newly failing
  cases (`test_after.log`)
- [x] Reduced the next `stl_iterator.h:1011` blocker to
  `tests/cpp/internal/postive_case/qualified_template_unresolved_param_type_parse.cpp`
  for `void accept(T& value, ns::holder<T> other);`, and confirmed it failed
  before implementation with the same top-level parameter-list `::` failure
- [x] Taught parameter-list entry probes to treat unresolved qualified
  template-id forms such as `ns::holder<T>` as valid parameter type starts,
  and re-ran the targeted parse tests:
  `qualified_template_unresolved_param_type_parse`,
  `template_unresolved_param_type_parse`,
  `qualified_cpp_base_type_dispatch_parse`,
  `qualified_type_start_shared_probe_parse`, and
  `qualified_global_type_start_shared_probe_parse`
- [x] Advanced the direct `std::vector` parse-only repro beyond the
  `stl_iterator.h:1011` `std::__detail::__range_iter_t<_Container>` frontier;
  the next failures now start at `/usr/include/c++/14/bits/stl_iterator.h:1954`
  and `:1978` on copy/move constructor parameter forms, followed by later
  allocator, incomplete-type, comma-expression, and attribute-related issues
- [x] Full-suite regression guard passed with monotonic improvement:
  `2386 -> 2387` passing tests, `0 -> 0` failing tests, and no newly failing
  cases (`test_after.log`)
- [x] Reduced the next `stl_iterator.h:1954` / `:1978` blocker to
  `tests/cpp/internal/postive_case/constructor_self_ref_param_parse.cpp` using
  `void accept(const Box& other, Box&& moved);`, and confirmed it failed before
  implementation with the same top-level parameter-list `&` / `&&` shape
- [x] Taught C++ parameter probes and base-type recovery to treat unresolved
  simple identifiers followed by `&` / `&&` as parameter type starts instead of
  K&R-style declarator names, and re-ran the targeted tests:
  `cpp_parse_constructor_self_ref_param_dump`,
  `cpp_positive_sema_constructor_self_ref_param_parse_cpp`,
  `cpp_positive_sema_qualified_template_unresolved_param_type_parse_cpp`,
  `cpp_positive_sema_cpp20_requires_clause_struct_decl_parse_cpp`,
  `cpp_parse_lambda_empty_capture_dump`, and
  `cpp_parse_lambda_empty_capture_parens_dump`
- [x] Advanced the direct `std::vector` parse-only repro beyond the
  `stl_iterator.h:1954` / `:1978` copy/move constructor frontier; the next
  failure now starts at `/usr/include/c++/14/bits/predefined_ops.h:150`
  (`parse_top_level_parameter_list` expected `)` got `__comp`), followed by
  allocator, incomplete-type, comma-expression, attribute, and later vector.tcc
  parameter-list issues
- [x] Full-suite regression guard passed with monotonic improvement:
  `2388 -> 2389` passing tests, `0 -> 0` failing tests, and no newly failing
  cases (`test_fail_before.log` vs `test_fail_after.log`)
- [x] Reduced the next `bits/predefined_ops.h:150` blocker to
  `tests/cpp/internal/postive_case/unresolved_by_value_param_type_parse.cpp`
  using `void accept(Value other);`, and confirmed it failed before
  implementation with the same top-level parameter-list `expected=RPAREN`
  shape on the parameter name
- [x] Taught C++ parameter probes and base-type recovery to treat unresolved
  simple identifiers followed by by-value declarator names as parameter type
  starts, and re-ran the targeted tests:
  `cpp_positive_sema_unresolved_by_value_param_type_parse_cpp`,
  `cpp_parse_unresolved_by_value_param_type_dump`,
  `cpp_positive_sema_constructor_self_ref_param_parse_cpp`, and
  `cpp_positive_sema_qualified_template_unresolved_param_type_parse_cpp`
- [x] Advanced the direct `std::vector` parse-only repro beyond the
  `predefined_ops.h:150` `_Compare __comp` frontier; the next failures now
  start at `/usr/include/c++/14/bits/stl_iterator.h:1572`,
  `:1593`, and `/usr/include/c++/14/bits/memoryfwd.h:68` on incomplete
  `move_iterator` / `allocator` types, followed by later comma-expression,
  attribute, and vector.tcc parser issues
- [x] Full-suite regression guard passed after this slice with no newly failing
  tests: checker reported `before: passed=661 failed=0 total=661`,
  `after: passed=2391 failed=0 total=2391`, `new failing tests: 0`, and
  `result: PASS`
- [x] Reduced the next `move_iterator` blocker to
  `tests/cpp/internal/postive_case/template_friend_record_member_parse.cpp`
  using a templated record with `template<typename U> friend class Box;`
  followed by an inline self-typed local declaration, and confirmed it failed
  before implementation with the same incomplete-type shape
- [x] Taught templated record-member parsing to re-run friend/static-assert
  preludes after consuming a member template prelude, so
  `template<...> friend ...;` no longer falls through into normal member
  parsing
- [x] Advanced the direct `std::vector` parse-only repro beyond the
  `bits/stl_iterator.h:1572` and `:1593` `move_iterator __tmp = *this`
  failures; the next surviving failure now starts at
  `/usr/include/c++/14/bits/memoryfwd.h:68` on `allocator<void>`, followed by
  the later comma-expression, attribute, and vector.tcc parser issues
- [x] Targeted parser/frontend checks passed for the new slice:
  `cpp_positive_sema_template_friend_record_member_parse_cpp`,
  `cpp_parse_template_friend_record_member_dump`,
  `cpp_positive_sema_constructor_self_ref_param_parse_cpp`,
  `cpp_parse_constructor_self_ref_param_dump`,
  `cpp_positive_sema_unresolved_by_value_param_type_parse_cpp`, and
  `cpp_parse_unresolved_by_value_param_type_dump`
- [x] Full-suite regression guard passed for the template-friend slice:
  `2391 -> 2393` passing tests, `0 -> 0` failing tests, and no newly failing
  cases (`test_fail_before.log` vs `test_fail_after.log`)
- [x] Reduced the `bits/memoryfwd.h:68` frontier to
  `tests/cpp/internal/postive_case/forward_declared_record_specialization_parse.cpp`
  and added `cpp_parse_forward_declared_record_specialization_dump` to guard
  the explicit-specialization forward-declaration AST shape
- [x] Taught record-specialization prebody/tag setup to treat forward
  declarations ending in `;` as explicit specializations and to preserve
  specialization metadata on forward record refs instead of treating them as
  plain object declarations
- [x] Advanced the direct `std::vector` parse-only repro beyond
  `/usr/include/c++/14/bits/memoryfwd.h:68` `allocator<void>`; the next
  failures now start at `/usr/include/c++/14/bits/stl_uninitialized.h:179`,
  `:247`, and `:317` on comma-expression parsing, followed by
  `stl_uninitialized.h:1109` (`[`), typedef-base failures in
  `functional_hash.h` and `stl_function.h`, and later `stl_bvector.h` /
  `vector.tcc` parameter-list frontiers
- [x] Targeted parser/frontend checks passed for the forward-declared
  specialization slice:
  `cpp_positive_sema_forward_declared_record_specialization_parse_cpp`,
  `cpp_parse_forward_declared_record_specialization_dump`,
  `cpp_positive_sema_record_specialization_setup_parse_cpp`, and
  `cpp_positive_sema_template_struct_specialization_parse_cpp`
- [x] Full-suite regression guard passed for the forward-declared
  specialization slice: `2391 -> 2395` passing tests, `0 -> 0` failing tests,
  and no newly failing cases (`test_fail_before.log` vs `test_fail_after.log`)

## Blockers

- none recorded yet

## Resume Notes

- Do not activate `ideas/open/__backend_port_plan.md` directly; it is an
  umbrella roadmap and explicitly says execution should activate a narrower idea.
- The source idea says earlier parser hazards were already retired; start from
  the current `noexcept` and `iterator_concepts.h` frontier instead of
  reworking the older blockers.
- The surviving direct repro errors now begin with comma-expression parsing in
  `bits/stl_uninitialized.h:179`, `:247`, and `:317`, then attribute parsing
  in `stl_uninitialized.h:1109`, typedef-base failures in
  `functional_hash.h` / `stl_function.h`, and several remaining
  `stl_bvector.h` / `vector.tcc` parameter-list issues.
