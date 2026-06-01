Status: Active
Source Idea Path: ideas/open/72_aarch64_special_carrier_prepared_policy_cleanup.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Acceptance Validation And Drift Check

# Current Packet

## Just Finished

Completed `plan.md` Step 6 review-blocker repair for AArch64 f128 runtime
helper provenance validation.

`src/backend/mir/aarch64/codegen/f128.cpp` now requires the selected f128
helper record policy to match its prepared `source_helper` policy for
resource, ABI, clobber, live-preservation, selected-call ownership,
full-width carrier/ABI/marshal, scalar ownership/marshal, and comparison
result-consumption facts that drive helper emission. Incomplete selected scalar
move facts still fall through to the existing incomplete-structured-field
diagnostics instead of being reclassified as source-policy drift.

`tests/backend/mir/backend_aarch64_machine_printer_test.cpp` now has a focused
negative fixture that mutates the selected record clobber policy after
attaching an independent prepared source helper and verifies fail-closed
behavior.

## Suggested Next

Next coherent packet: supervisor should review the Step 6 acceptance repair
diff and decide whether to commit or request another reviewer pass.

## Watchouts

`review/idea69_step2_prepared_effect_review.md` and
`review/idea70_steps1_3_review.md` were already untracked and were not touched.
`review/idea72_steps1_5_acceptance_review.md` is also untracked and was not
touched. `review/idea72_step6_acceptance_repair_review.md` was also untracked
and was not touched.

## Proof

Delegated proof passed:
`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_machine_printer|backend_aarch64_target_instruction_records)$' >> test_after.log 2>&1`.

`backend_aarch64_machine_printer` passed.
`backend_aarch64_target_instruction_records` passed.
Proof log: `test_after.log`.
