# Current Packet

Status: Active
Source Idea Path: ideas/open/123_prepared_call_result_late_publication_contract.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Shared Call-Result Late-Publication Fact Or Query

## Just Finished

Completed plan Step 2: added a shared target-neutral
`PreparedCallResultLatePublicationFact` query over `PreparedCallResultPlan`
without moving AArch64 emission behavior.

Changed files:
- `src/backend/prealloc/calls.hpp`: added
  `find_prepared_call_result_late_publication`, exposing availability for
  source-register result publication, source-in-destination aliasing,
  and FPR/VREG store-value retargeting from prepared result-plan fields.
  `current_block_publication_consumption_available` remains false because
  this Step 2 surface is not tied to an existing prepared current-block
  publication fact.
- `src/backend/prealloc/prepared_printer/calls.cpp`: prints the
  late-publication fact on prepared call-result dump lines.
- `tests/backend/bir/backend_prepare_frame_stack_call_contract_test.cpp`:
  covers source-register visibility on a stack-backed result, verifies that
  current-block publication is not overclaimed, covers FPR retargeting
  visibility on a float result, and includes a focused alias query fixture.

AArch64 FP/f128/GP emission, scratch policy, q/vector spelling, memory-store
record mutation, and call-boundary machine record construction remain
target-local; no AArch64 source file was changed in this packet.

## Suggested Next

Execute Step 3 by converting the AArch64 call-result late-publication
consumers to consult the shared query where it describes target-neutral
availability, while keeping concrete materialization, scalar-state mutation,
and machine-record emission local.

## Watchouts

- The new query answers availability from prepared result-plan facts only; it
  intentionally does not decide AArch64 register views, scratch registers,
  f128 carrier transport, or memory-store record mutation.
- `current_block_publication_consumption_available` is intentionally not
  claimed by this query yet. Representing it accurately needs a broader
  signature tied to the existing prepared current-block publication/producer
  facts rather than destination identity on `PreparedCallResultPlan`.
- `source_in_destination_alias_available` is true only when the prepared
  source and destination register names/banks are both present and equal.
- Do not reopen idea 117 current-block publication authority or idea 116
  source-producer authority while consuming this surface.

## Proof

Ran the delegated proof exactly:
`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_frame_stack_call_contract|backend_prealloc_call_boundary_classification|backend_call_boundary_effect_plan|backend_aarch64_call_boundary_owner|backend_aarch64_instruction_dispatch|backend_aarch64_target_instruction_records)$' >> test_after.log 2>&1`

Result: passed. `test_after.log` shows 6/6 tests passed:
`backend_prepare_frame_stack_call_contract`,
`backend_prealloc_call_boundary_classification`,
`backend_call_boundary_effect_plan`,
`backend_aarch64_call_boundary_owner`,
`backend_aarch64_instruction_dispatch`, and
`backend_aarch64_target_instruction_records`.

Supervisor regression guard:
`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`
passed with 6/6 before and 6/6 after, no new failures.
