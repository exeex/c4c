# Active Plan Todo

Status: Active
Source Idea: ideas/open/04_std_vector_bringup_plan.md
Source Plan: plan.md
Last Updated: 2026-03-30

## Current Active Item

- Step 2/3: reduce and fix the surviving `max_size_type.h` parser frontier,
  currently anchored at `/usr/include/c++/14/bits/max_size_type.h:564`
  `try_parse_record_constructor_member expected=RPAREN got='-'`

## Todo

- [x] Add one internal parse-only testcase for
      `typename __allocator_traits_base::template __rebind<_Alloc, _Up>::type`
      reduced from `/usr/include/c++/14/bits/alloc_traits.h:134`
- [x] Implement the smallest parser fix required by that reduced testcase
- [x] Re-run the direct `std::vector` repro to confirm the next frontier after
      the `alloc_traits` dependent-template slice
- [x] Validate the next completed slice with targeted tests plus a monotonic
      full-suite comparison
- [ ] Reduce the contextual `max_size_type.h:564` failure to the smallest
      committed internal testcase
- [ ] Implement the narrowest parser fix for the reduced `max_size_type`
      frontier
- [ ] Re-run the direct `std::vector` repro to confirm the next post-
      `max_size_type` frontier
- [ ] Validate the completed `max_size_type` slice with targeted tests plus a
      monotonic full-suite comparison

## Completed

- [x] Selected `ideas/open/04_std_vector_bringup_plan.md` as the sole
      activation target after closing the SFINAE parser-coverage idea
- [x] Confirmed there was no active `plan.md` / `plan_todo.md` pair to
      overwrite
- [x] Generated `plan.md` as an execution-oriented runbook linked to the
      parked `std::vector` bring-up idea
- [x] Reconfirmed the live `tests/cpp/std/std_vector_simple.cpp` parse-only
      frontier and identified a reducible earlier blocker in
      `/usr/include/c++/14/bits/max_size_type.h` around `if (...) [[likely]]`
      statement attributes
- [x] Added reduced parse-only coverage in
      `tests/cpp/internal/postive_case/cpp20_if_likely_stmt_attr_parse.cpp`
- [x] Fixed `if` statement parsing to skip statement attributes between the
      condition and the controlled statement
- [x] Re-ran the direct `std::vector` repro and confirmed the
      `bits/max_size_type.h` `/=` / `else` failures disappeared
- [x] Validated the slice with targeted tests and a monotonic full-suite
      comparison (`2431/2432` passed before, `2432/2433` passed after; no new
      failing tests)
- [x] Reduced the surviving `type_traits` dependent `::template` alias failure
      into `tests/cpp/internal/postive_case/dependent_template_typename_member_parse.cpp`
- [x] Fixed qualified `typename` type spelling to continue through
      `::template member<...>` in the reduced `type_traits` alias case
- [x] Re-ran the direct `std::vector` repro and confirmed
      `/usr/include/c++/14/type_traits:157` disappeared, leaving
      `/usr/include/c++/14/bits/alloc_traits.h:134` as the lead
      dependent-template frontier
- [x] Validated the reduced `type_traits` slice with targeted parser tests and
      a monotonic full-suite comparison (`2431/2432` passed before,
      `2434/2434` passed after; no new failing tests)
- [x] Reduced the surviving `alloc_traits` dependent member-template alias
      frontier into
      `tests/cpp/internal/postive_case/record_base_template_member_alias_parse.cpp`
- [x] Fixed top-level `using` alias recovery so scoped `::template` segments do
      not get misclassified as a new declaration boundary
- [x] Re-ran the direct `std::vector` repro and confirmed
      `/usr/include/c++/14/bits/alloc_traits.h:134` disappeared, exposing a
      later frontier in `/usr/include/c++/14/bits/max_size_type.h`
- [x] Validated the reduced `alloc_traits` slice with nearby parser tests and a
      monotonic full-suite comparison (`2434/2435` passed before,
      `2435/2435` passed after; no new failing tests)
- [x] Reduced the `max_size_type.h` frontier enough to show
      `bits/max_size_type.h` does not fail in isolation; the failure appears
      once iterator/concepts headers such as `bits/stl_iterator.h` are parsed
      before it
- [x] Fixed local incomplete-type checking so a forward-declared record is
      treated as complete for self-type local declarations inside its own
      inline member bodies once the full definition is being parsed
- [x] Added parser regressions for namespaced forward-declared self-type local
      declarations, including a fully-qualified spelling variant, plus a
      negative guard that keeps unrelated forward-declared local object types
      rejected

## Next Intended Slice

- Reduce the next `std::vector` frontier in
  `/usr/include/c++/14/bits/max_size_type.h`, still starting from the
  `try_parse_record_constructor_member expected=RPAREN got='-'` failure at
  line 564; this iteration confirmed the newer `operator<=>` parser support is
  not sufficient to clear that frontier on its own.

## Blockers

- The direct repro now clears `alloc_traits.h:134`, but the next
  `max_size_type.h` frontier is not reduced yet; the first surviving parse-side
  anchor is the record-constructor-member `got='-'` failure at line 564, with
  incomplete-type diagnostics appearing earlier in the same header.
- The `__max_size_type` local incomplete-type false positive is fixed, but the
  remaining `__max_diff_type` cascade shows there is still a second parser bug
  in the `max_size_type.h` / `ranges_base.h` path after forward-declare
  handling.
- This iteration added lexer/parser support for the C++20 `<=>` token plus
  basic member-operator parsing coverage, but the real `max_size_type.h:564`
  blocker still reproduces after `<bits/stl_iterator.h>` and
  `<bits/max_size_type.h>` are combined, so another contextual parser issue
  remains.

## Resume Notes

- Keep this plan narrow to one live `std::vector` parser blocker at a time.
- This iteration confirmed the reduced `alloc_traits` alias case was not a
  qualified-type parse failure; it was a false positive in
  `using_alias_consumed_following_declaration(...)`, which treated scoped
  `::template` as a new top-level declaration boundary.
- Earlier completed slices already landed reduced coverage for template-template
  parameters, qualified NTTPs, dependent enum `sizeof(...)` initializers,
  access-label handling, friend declarations, `noexcept(expr)`, constrained
  declaration boundaries, unresolved qualified template-id parameter types,
  unresolved self-type parameters, forward-declared explicit specializations,
  and GCC/Clang type-trait builtin type arguments.
- This slice landed `if (...) [[likely]]` statement-attribute support and added
  the corresponding reduced parser testcase.
- This slice also landed reduced coverage for dependent
  `typename X<...>::template member<...>` alias parsing and moved the direct
  `std::vector` frontier from `type_traits.h:157` to `alloc_traits.h:134`.
- This slice landed reduced coverage for record-qualified
  `typename X::template member<...>::type` aliases and moved the direct
  `std::vector` frontier past `alloc_traits.h:134` into `max_size_type.h`.
- The current `max_size_type.h` investigation showed the first forward-declare
  related bug was in local incomplete-type checking for inline member bodies;
  targeted internal tests now cover both unqualified and fully-qualified
  self-type locals after a prior forward declaration.
- This iteration also added basic lexer/parser support for `operator<=>`,
  including member parse coverage and a reduced include-order testcase
  (`stl_iterator_then_max_size_type_parse.cpp`) that locks in the still-failing
  contextual `max_size_type` frontier.
- Do not reactivate `ideas/open/__backend_port_plan.md`; it is an umbrella
  roadmap, not the next execution target.
