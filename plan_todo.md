# Active Todo

Status: Active
Source Idea: ideas/open/04_std_vector_bringup_plan.md
Source Plan: plan.md

## Current Active Item

- Step 2: reduce the next concept-heavy `iterator_concepts.h` blocker exposed
  after the `noexcept(expr)` parser fix

## Next Intended Slice

- inspect `/usr/include/c++/14/bits/iterator_concepts.h:75` and isolate the
  first `unexpected token in expression: struct` family into a standalone parse
  test
- keep the next reduced case focused on one root cause from the surviving
  `struct` / `typename` / `...` iterator-concepts frontier
- rerun `./build/c4cll --parse-only tests/cpp/std/std_vector_simple.cpp` after
  that next fix to confirm the frontier advances again

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

## Blockers

- none recorded yet

## Resume Notes

- Do not activate `ideas/open/__backend_port_plan.md` directly; it is an
  umbrella roadmap and explicitly says execution should activate a narrower idea.
- The source idea says earlier parser hazards were already retired; start from
  the current `noexcept` and `iterator_concepts.h` frontier instead of
  reworking the older blockers.
- Reduced repro candidate for this slice:
  `constexpr bool probe() { return noexcept(foo()); }`
- The surviving direct repro errors now begin with
  `iterator_concepts.h:75` (`struct`), then `typename`, `...`, reference-qualifier
  parameter parsing in `bits/exception.h`, and later `constexpr` / comma forms
  in `bits/stl_iterator.h`.
