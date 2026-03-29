# Active Plan Todo

Status: Active
Source Idea: ideas/open/06_sfinae_template_parameter_and_signature_patterns.md
Source Plan: plan.md
Last Updated: 2026-03-29

## Current Active Item

- Step 3: choose the first narrow function-signature SFINAE slice now that
  template-parameter-list coverage shows no remaining parser-only gap

## Todo

- [x] Inventory existing parser tests that already overlap the six target
      families
- [x] Add or stage reduced tests so each target family has concrete testcase
      coverage
- [x] Record the first parser failure signature for each still-failing family
- [x] Implement the first narrow template-parameter parsing slice
- [x] Validate targeted tests plus full-suite monotonicity after the Step 2
      decision slice
- [ ] Inventory which function-signature SFINAE forms still need parser work
      rather than later semantic staging
- [ ] Implement the first narrow function-signature normalization slice

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
- [x] Added
      `tests/cpp/internal/postive_case/member_template_sfinae_typename_prelude_parse.cpp`
      to prove dependent `typename ...::type` still parses cleanly inside
      member-template preludes for both defaulted type parameters and typed
      NTTPs
- [x] Confirmed the added member-template probe plus adjacent prelude/template
      regressions pass without any parser change in `src/frontend/parser`
- [x] Validated the Step 2 decision slice with a monotonic full-suite
      comparison (`2428/2429` -> `2429/2430`, no newly failing tests; existing
      baseline failure unchanged at
      `cpp_positive_sema_iterator_concepts_following_hash_base_parse_cpp`)

## Next Intended Slice

- Re-run the reduced return-type and parameter-type SFINAE probes to determine
  whether Step 3 still has a parser gap or only later semantic fallout.

## Blockers

- None recorded at activation time.

## Resume Notes

- Keep this plan narrow to parser coverage and staged semantic fallback.
- Current iteration target:
  Step 2 is closed by coverage confirmation rather than parser edits:
  the added member-template `typename ...::type` probe passed immediately, so
  the next iteration should pivot to Step 3 function-signature forms.
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
- Step 2 closure evidence:
  `cpp_positive_sema_member_template_sfinae_typename_prelude_parse_cpp`,
  `cpp_positive_sema_sfinae_template_parameter_patterns_parse_cpp`,
  `cpp_positive_sema_template_typename_typed_nttp_parse_cpp`, and
  `cpp_positive_sema_eastl_probe_qualified_template_scope_parse_cpp` all pass,
  so no template-parameter parser patch is justified from the current reduced
  corpus.
- Do not activate `ideas/open/__backend_port_plan.md`; it is an umbrella
  roadmap, not the direct target for this runbook.
- The parked `std::vector` bring-up idea remains adjacent follow-on work, not
  part of this active plan unless a later lifecycle switch says otherwise.
