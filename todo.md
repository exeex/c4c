# Current Packet

Status: Active
Source Idea Path: ideas/open/123_prepared_call_result_late_publication_contract.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Add FPR/VREG Store-Value Retargeting Visibility

## Just Finished

Completed plan Step 4: converted FPR/VREG call-result store-value retarget
availability to consult the shared `PreparedCallResultLatePublicationFact`
where the target-neutral prepared identity is available.

Changed files:
- `src/backend/mir/aarch64/codegen/calls.cpp`: added a narrow
  `call_result_store_value_retarget_available` bridge that builds the shared
  late-publication query from an emitted CallAbi scalar register carrying
  prepared value id and prepared bank. `retarget_fpr_call_result_store_value_to_emitted_scalar`
  now consults that bridge before retargeting an FPR/VREG call-result store
  value when the shared facts are present.

`MemoryInstructionRecord` mutation, AArch64 FpSimd bank checks, register view
selection, q/vector spelling, and f128 handling remain local. The helper
preserves the old fallback for emitted scalar records that do not carry enough
prepared identity for the shared query to represent the target-neutral part.

## Suggested Next

Execute the next packet by addressing the remaining current-block publication
visibility gap with a surface that explicitly accepts or references existing
prepared current-block publication/producer facts, without deriving that
authority from call-result destination identity.

## Watchouts

- `current_block_publication_consumption_available` is still intentionally
  unclaimed by `find_prepared_call_result_late_publication`; do not infer it
  from `PreparedCallResultPlan` destination fields.
- The FPR/VREG retarget bridge only gates availability. It does not move store
  record mutation, register-bank/view validation, or f128/q-register spelling
  into shared prepared code.
- If a future packet wants to remove the fallback for missing prepared identity,
  first prove every emitted CallAbi FPR/VREG scalar that reaches the store
  retarget path carries a prepared value id and prepared bank.

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
