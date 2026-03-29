# Active Plan Todo

Status: Active
Source Idea: ideas/open/04_std_vector_bringup_plan.md
Source Plan: plan.md
Last Updated: 2026-03-29

## Current Active Item

- Step 2: reduce the current `type_traits` / `alloc_traits` dependent-template
  alias frontier into one internal parser testcase before touching parser code

## Todo

- [ ] Reduce `/usr/include/c++/14/type_traits:157` and/or
      `/usr/include/c++/14/bits/alloc_traits.h:134` into one internal
      parse-only testcase for dependent `::template` alias/member lookup
- [ ] Implement the smallest parser fix required by that reduced testcase
- [ ] Re-run the direct `std::vector` repro to confirm the next frontier after
      the dependent-template alias slice
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

## Next Intended Slice

- Reduce the surviving dependent-template alias/member lookup failures at
  `/usr/include/c++/14/type_traits:157` and
  `/usr/include/c++/14/bits/alloc_traits.h:134`; they now precede the
  `ranges_base.h` `[[nodiscard]]` record-member frontier in the direct
  `std::vector` repro.

## Blockers

- The smallest standalone repro for the `type_traits` /
  `alloc_traits` `::template` failures is not isolated yet; simple synthetic
  reductions parse successfully, so the next slice needs a tighter reduction
  from the real headers.

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
- Do not reactivate `ideas/open/__backend_port_plan.md`; it is an umbrella
  roadmap, not the next execution target.
