# Plan Todo

Status: Active
Source Idea: ideas/open/std_vector_success_path_and_stl_throughput_follow_on.md
Source Plan: plan.md

## Active Item

- Step 1: Baseline motivating workflows.

## Checklist

- [ ] Measure bounded `--parse-only` behavior on
      `tests/cpp/std/std_vector_simple.cpp`.
- [ ] Measure bounded parser-debug behavior on `std_vector_simple.cpp` only if
      it adds decision-making value beyond parse-only timing.
- [ ] Confirm the manual include-path recipe for
      `tests/cpp/eastl/eastl_vector_simple.cpp`.
- [ ] Record the current EASTL failure phase and the exact failing location.
- [ ] Update todo state with the measured baseline and choose the next slice.

## Completed

- [x] Closed the completed parser-debug observability runbook and archived its
      source idea under `ideas/closed/`.
- [x] Recorded this follow-on initiative as a separate open idea instead of
      silently expanding the completed runbook.
- [x] Activated `ideas/open/std_vector_success_path_and_stl_throughput_follow_on.md`
      into `plan.md`.

## Next Intended Slice

- Finish the Step 1 baseline using the already observed signals:
  `std_vector_simple.cpp` exceeded a 60s bounded `--parse-only` run in this
  environment, and `eastl_vector_simple.cpp` reaches a committed parser failure
  in `EASTL/internal/type_properties.h` once invoked with
  `-I ref/EASTL/include -I ref/EABase/include/Common`.

## Blockers

- None yet. The current state is measurement work, not an implementation
  blocker.

## Resume Notes

- The previous runbook already proved that EASTL motivating failures benefit
  from token-window and tentative trace output, so this plan should not revisit
  those debug payload changes unless a new measurement shows a direct gap.
- Keep the next slice narrow: success-path summary, throughput, or workflow
  wrapper, but not all three at once.
