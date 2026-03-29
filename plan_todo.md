# Active Todo

Status: Active
Source Idea: ideas/open/04_std_vector_bringup_plan.md
Source Plan: plan.md

## Current Active Item

- Step 2: reduce the next post-`iterator_concepts` blocker, starting with the
  `stl_iterator.h:1011` parameter-list failure on
  `std::__detail::__range_iter_t<_Container> __i`

## Next Intended Slice

- inspect `/usr/include/c++/14/bits/stl_iterator.h:1011` and isolate the
  smallest parse-only repro for a parameter type spelled as
  `std::__detail::__range_iter_t<_Container>`
- keep the next fix limited to top-level/member parameter-list parsing rather
  than broad iterator or allocator semantics
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

## Blockers

- none recorded yet

## Resume Notes

- Do not activate `ideas/open/__backend_port_plan.md` directly; it is an
  umbrella roadmap and explicitly says execution should activate a narrower idea.
- The source idea says earlier parser hazards were already retired; start from
  the current `noexcept` and `iterator_concepts.h` frontier instead of
  reworking the older blockers.
- Reduced repro candidate for this slice:
  `template<typename T> requires is_object_v<T> struct trait;`
- The surviving direct repro errors now begin with
  `stl_iterator.h:1011` (`parse_top_level_parameter_list` expected `)` got
  `::`), then copy/move constructor parameter forms in `stl_iterator.h`,
  allocator/predefined-ops parameter-list issues, incomplete-type reports for
  `move_iterator` / `allocator`, and later comma-expression parsing in
  `bits/stl_uninitialized.h`.
