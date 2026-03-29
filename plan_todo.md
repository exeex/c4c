# Active Plan Todo

Status: Active
Source Idea: ideas/open/04_std_vector_bringup_plan.md
Source Plan: plan.md
Last Updated: 2026-03-29

## Current Active Item

- Step 2: reduce `/usr/include/c++/14/bits/alloc_traits.h:134` into one
  internal parser testcase for the surviving record-base dependent
  `::template __rebind<...>::type` frontier

## Todo

- [ ] Reduce `/usr/include/c++/14/bits/alloc_traits.h:134` into one internal
      parse-only testcase for record-base dependent
      `typename __allocator_traits_base::template __rebind<...>::type`
- [ ] Implement the smallest parser fix required by that reduced testcase
- [ ] Re-run the direct `std::vector` repro to confirm the next frontier after
      the `alloc_traits` dependent-template slice
- [ ] Validate the next completed slice with targeted tests plus a monotonic
      full-suite comparison

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

## Next Intended Slice

- Reduce the surviving dependent-template member-template failure at
  `/usr/include/c++/14/bits/alloc_traits.h:134`; it now precedes the
  `ranges_base.h` `[[nodiscard]]` record-member frontier in the direct
  `std::vector` repro.

## Blockers

- The `alloc_traits.h:134` form still is not isolated cleanly: the parser now
  accepts the `type_traits` alias form, but the record-base
  `typename __allocator_traits_base::template __rebind<...>::type` shape still
  needs its own reduced testcase and parser entry-point fix.

## Resume Notes

- Keep this plan narrow to one live `std::vector` parser blocker at a time.
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
- Do not reactivate `ideas/open/__backend_port_plan.md`; it is an umbrella
  roadmap, not the next execution target.
