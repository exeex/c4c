# Object Route Scan And Default Readiness

## Goal

Broaden object-route validation across backend and c-testsuite coverage, then
decide whether `.o` should become the default backend output form or remain a
documented dual-route option.

## Why This Exists

Narrow target and CLI object tests prove the route exists, but default-readiness
requires broader evidence. The project needs a deliberate scan that keeps asm
coverage meaningful, finds object-specific gaps, and prevents unsupported or
flaky cases from being hidden by expectation churn.

## In Scope

- Add c-testsuite or backend scan labels for object-route execution beside
  existing asm-route labels.
- Run and triage object-route scan results for supported RV64 and AArch64
  subsets.
- Record any remaining unsupported cases as focused follow-up ideas rather than
  weakening broad expectations.
- Compare asm-route and object-route behavior for representative runtime and
  linkability coverage.
- Decide, with recorded proof, whether object output is ready to become the
  default backend route or should remain dual-route for now.

## Out Of Scope

- Implementing the shared object model or target object encoders.
- Adding the initial `--codegen obj` CLI wiring.
- Removing asm-route coverage after object-route success.
- Fixing every discovered backend semantic bug inside this scan child.
- Building textual assembler support.

## Acceptance And Proof Expectations

- Object-route scans can be selected independently and produce stable,
  diagnosable results.
- The scan has before/after proof for the chosen supported subset and no new
  asm-route regressions.
- Any unresolved object-route failures are either fixed in focused slices or
  moved into new child ideas with concrete ownership.
- The default-route recommendation is recorded with dates, commands, result
  counts, and rationale.

## Dependency Notes

- Depends on target object-emission children and CLI/test integration:
  `ideas/open/331_rv64_minimal_relocatable_elf_object_emission.md`,
  `ideas/open/332_aarch64_minimal_relocatable_elf_object_emission.md`, and
  `ideas/open/333_codegen_obj_cli_and_test_integration.md`.
- This child should run after narrow object-route proof exists, not as the
  first implementation slice.
- May create additional focused follow-up ideas for specific object-route
  failures discovered during scanning.

## Resume State

Resumed after focused child
`ideas/closed/336_target_object_emitter_scalar_scan_expansion.md` repaired the
target-owned scalar object-emitter gaps that parked this umbrella.

The first scalar object-route scan baseline is now green:

- RV64 object runtime candidates `return_add.c`, `two_arg_helper.c`,
  `return_add_sub_chain.c`, and `local_temp.c` are restored as object-route
  runtime scan cases beside their asm-route counterparts.
- AArch64 object byte candidates `return_add.c` and
  `return_add_sub_chain.c` are restored as object-byte scan cases beside the
  AArch64 asm external return smokes.
- Matching 28-test restored scan proof passed before and after, with the
  non-decreasing regression guard passing.

Continue this umbrella from the green scalar scan baseline. Broader object
scan/default-readiness work should add or triage additional object-route cases
without weakening existing asm-route or object-route expectations.

## Parked For Focused Child 337

The first resumed Step 1 inspection found that the next obvious scan candidates
are not harness-only additions yet:

- RV64 local-backed scalar call arguments in `two_arg_local_arg.c`,
  `two_arg_second_local_arg.c`, `two_arg_both_local_arg.c`, and
  `local_arg_call.c` still reject at target object emission.
- AArch64 `two_arg_helper.c` still needs selected register-register scalar or
  call-result object support.
- AArch64 local/frame-memory cases remain a later boundary.

Idea `ideas/open/337_target_object_emitter_local_call_and_regreg_scalar_expansion.md`
is activated to repair those target-owned gaps before this scan/default
readiness umbrella resumes. Keep the 28/28 restored scalar proof as the parent
baseline while 337 is active.

## Reviewer Reject Signals

- The scan marks failing cases unsupported, downgrades expectations, or narrows
  test contracts without explicit approval and a separate follow-up plan.
- The route claims default readiness based only on one narrow smoke case or one
  target while the other target remains unexamined.
- Existing asm-route coverage is deleted or weakened because object-route tests
  pass.
- Object-route failures are hidden behind generic clang/linker failures instead
  of being triaged to encoder, object writer, relocation, CLI, or runtime
  layers.
- The default switch is made without recorded scan commands, result counts, and
  regression comparison.
- New failures are absorbed into this umbrella silently instead of becoming
  focused follow-up ideas when they are distinct backend semantic bugs.
