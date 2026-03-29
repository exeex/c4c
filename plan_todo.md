# Active Plan Todo

Status: Active
Source Idea: ideas/open/06_sfinae_template_parameter_and_signature_patterns.md
Source Plan: plan.md
Last Updated: 2026-03-29

## Current Active Item

- Step 2: decide whether any template-parameter-list SFINAE family still needs
  a parser change now that reduced coverage exists for all six runbook targets

## Todo

- [x] Inventory existing parser tests that already overlap the six target
      families
- [x] Add or stage reduced tests so each target family has concrete testcase
      coverage
- [x] Record the first parser failure signature for each still-failing family
- [ ] Implement the first narrow template-parameter parsing slice
- [ ] Validate targeted tests plus full-suite monotonicity after the first
      implementation patch

## Completed

- [x] Selected
      `ideas/open/06_sfinae_template_parameter_and_signature_patterns.md` as
      the sole activation target
- [x] Confirmed there was no active `plan.md` / `plan_todo.md` pair to
      overwrite
- [x] Generated `plan.md` as an execution-oriented runbook linked to the source
      idea
- [x] Mapped existing overlap to
      `eastl_probe_qualified_template_scope_parse.cpp`,
      `template_typename_typed_nttp_parse.cpp`, and
      `template_angle_bracket_validation.cpp`
- [x] Added
      `tests/cpp/internal/postive_case/sfinae_template_parameter_patterns_parse.cpp`
      to give one reduced parse testcase covering all six runbook pattern
      families, including record-member template preludes
- [x] Confirmed by direct reduced probes that the return-type, parameter-type,
      and specialization-gating families currently reach normal parse output
      rather than a parser failure signature
- [x] Validated the coverage-only slice with targeted `ctest` plus a monotonic
      full-suite before/after comparison (`2427/2428` -> `2428/2429`, no newly
      failing tests)

## Next Intended Slice

- Inspect whether any still-missing behavior in the template-parameter-list
  families is semantic-only rather than syntactic before forcing a parser patch.

## Blockers

- None recorded at activation time.

## Resume Notes

- Keep this plan narrow to parser coverage and staged semantic fallback.
- Step 1 inventory result:
  unnamed typed NTTP already overlapped
  `eastl_probe_qualified_template_scope_parse.cpp`;
  unnamed type-parameter default already overlapped
  `template_typename_typed_nttp_parse.cpp`;
  return-type SFINAE already overlapped
  `template_angle_bracket_validation.cpp`;
  named typed NTTP, parameter-type SFINAE, and specialization-gating did not
  have an equally reduced committed case before this iteration.
- First parser-failure signature inventory:
  none observed in current reduced probes for any of the six target families;
  the active gap after this iteration is reduced coverage, not a reproduced
  parser break in the narrowed corpus.
- Do not activate `ideas/open/__backend_port_plan.md`; it is an umbrella
  roadmap, not the direct target for this runbook.
- The parked `std::vector` bring-up idea remains adjacent follow-on work, not
  part of this active plan unless a later lifecycle switch says otherwise.
