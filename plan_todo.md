# Active Plan Todo

Status: Active
Source Idea: ideas/open/04_std_vector_bringup_plan.md
Source Plan: plan.md
Last Updated: 2026-03-29

## Current Active Item

- Step 1: rerun the direct `std::vector` parse-only repro and record the first
  surviving parser blocker before adding any new reduced testcase

## Todo

- [ ] Reconfirm the live `tests/cpp/std/std_vector_simple.cpp` parse-only
      frontier
- [ ] Reduce the current lead blocker to one internal parser testcase
- [ ] Implement the smallest parser fix required by that reduced testcase
- [ ] Re-run the direct `std::vector` repro to record the next frontier
- [ ] Validate the completed slice with targeted tests plus a monotonic
      full-suite comparison

## Completed

- [x] Selected `ideas/open/04_std_vector_bringup_plan.md` as the sole
      activation target after closing the SFINAE parser-coverage idea
- [x] Confirmed there was no active `plan.md` / `plan_todo.md` pair to
      overwrite
- [x] Generated `plan.md` as an execution-oriented runbook linked to the
      parked `std::vector` bring-up idea

## Next Intended Slice

- Capture the exact current `std::vector` parse-only error stack and choose the
  first blocker worth reducing, with priority on the surviving
  `iterator_concepts` / concept-heavy declaration frontier unless the live repro
  shows a new earlier failure.

## Blockers

- None yet; the first repro refresh for this activation still needs to run.

## Resume Notes

- Keep this plan narrow to one live `std::vector` parser blocker at a time.
- Earlier completed slices already landed reduced coverage for template-template
  parameters, qualified NTTPs, dependent enum `sizeof(...)` initializers,
  access-label handling, friend declarations, `noexcept(expr)`, constrained
  declaration boundaries, unresolved qualified template-id parameter types,
  unresolved self-type parameters, forward-declared explicit specializations,
  and GCC/Clang type-trait builtin type arguments.
- Do not reactivate `ideas/open/__backend_port_plan.md`; it is an umbrella
  roadmap, not the next execution target.
