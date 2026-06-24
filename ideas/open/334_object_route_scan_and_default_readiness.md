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

## Resume State After Focused Child 337

Resumed after focused child
`ideas/closed/337_target_object_emitter_local_call_and_regreg_scalar_expansion.md`
repaired the next target-owned object-emitter gaps found by the first resumed
scan inspection.

The expanded scalar/local-call object-route scan baseline is now green:

- RV64 object runtime candidates `two_arg_local_arg.c`,
  `two_arg_second_local_arg.c`, `two_arg_both_local_arg.c`, and
  `local_arg_call.c` are restored as native object-route runtime scan cases
  beside their asm-route counterparts.
- AArch64 object byte candidate `two_arg_helper.c` is restored as an
  object-byte scan case beside the AArch64 asm scalar smokes.
- Expanded proof command:
  `set -o pipefail; (cmake --build --preset default && ctest --test-dir build -R '^(backend_object_model_records|backend_riscv_object_emission|backend_aarch64_object_emission|backend_cli_.*obj|backend_obj_runtime_.*|backend_rv64_runtime_(return_zero|return_add|two_arg_helper|two_arg_local_arg|two_arg_both_local_arg|two_arg_second_local_arg|local_arg_call|return_add_sub_chain|local_temp)|backend_cli_aarch64_asm_external_return_(zero|add|add_sub_chain)_smoke|backend_codegen_route_riscv64_external_no_storage_main_emits_return_path)$' --output-on-failure) > test_after.log 2>&1`
- Result: 33/33 tests passed, with non-decreasing regression guard against the
  rolled baseline.

Continue this umbrella from the 33/33 expanded green baseline. Broader
globals/data, arrays, pointers, aggregates, broad memory/control flow,
broad AArch64 frame/local-memory expansion, AArch64 runtime, x86 object output,
object stdout, c-testsuite object defaults, and semantic-BIR object mode remain
outside the completed child and should be handled deliberately by scan results
or focused follow-up ideas.

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
