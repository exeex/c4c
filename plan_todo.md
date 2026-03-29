# Active Todo

Status: Active
Source Idea: ideas/open/04_std_vector_bringup_plan.md
Source Plan: plan.md

## Current Active Item

- Step 2: reduce the next post-`stl_iterator.h:1011` blocker, starting with
  the `stl_iterator.h:1954` / `:1978` copy/move constructor parameter-list
  failures on `const common_iterator& __x` and `common_iterator&& __x`

## Next Intended Slice

- inspect `/usr/include/c++/14/bits/stl_iterator.h:1954` and `:1978` and
  isolate the smallest parse-only repro for constructor parameters spelled as
  `const common_iterator&` and `common_iterator&&`
- keep the next fix limited to parameter declarator parsing for reference and
  rvalue-reference forms rather than broad iterator semantics
- rerun `./build/c4cll --parse-only tests/cpp/std/std_vector_simple.cpp` after
  that fix to confirm the frontier advances beyond the current
  `parse_top_level_parameter_list` failures

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

## Blockers

- none recorded yet

## Resume Notes

- Do not activate `ideas/open/__backend_port_plan.md` directly; it is an
  umbrella roadmap and explicitly says execution should activate a narrower idea.
- The source idea says earlier parser hazards were already retired; start from
  the current `noexcept` and `iterator_concepts.h` frontier instead of
  reworking the older blockers.
- Reduced repro candidate for the next slice:
  a minimal constructor or free-function case exercising
  `const common_iterator&` / `common_iterator&&` parameter declarators
- The surviving direct repro errors now begin with
  `stl_iterator.h:1954` (`parse_top_level_parameter_list` expected `)` got
  `&`) and `stl_iterator.h:1978` (`... got '&&'`), then
  allocator/predefined-ops parameter-list issues, incomplete-type reports for
  `move_iterator` / `allocator`, and later comma-expression parsing in
  `bits/stl_uninitialized.h`.
