# Current Packet

Status: Complete
Source Idea Path: ideas/open/158_bir_comparison_condition_producer_identity.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Switch comparison and branch provenance consumers

## Just Finished

Step 5 completed the cast-operand load-producer read switch inside
`lower_fused_compare_branch_from_emitted_cast` from
`find_prepared_fused_compare_operand_producer` to
`bir::find_comparison_operand_producer` for `cast->operand` at
`cast_producer->producer_instruction_index`.

The BIR producer path now requires
`bir::ComparisonProducerKind::LoadLocal`, recovers the `bir::LoadLocalInst`,
keeps the existing `I8` source-load type check, and still uses the producer
instruction index for `branch_fusion_prepared_frame_slot_load_address`.

## Suggested Next

Supervisor can decide whether Step 5 needs another provenance-consumer packet
or whether the completed Step 5 slices are ready for the next plan
step/acceptance validation.

## Watchouts

- The requested cast-operand load lookup now uses BIR producer identity; do not
  restore the prepared load-producer read in a later cleanup.
- Do not move branch legality, cast opcode/type checks, condition-code
  selection, compare emission, branch emission, final-record policy, hazards,
  emitted-register state, RHS constant fallback, or load-address policy while
  continuing Step 5.
- This slice did not add or change test expectations.

## Proof

Delegated proof passed:
`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^backend_' >> test_after.log 2>&1`.

`test_after.log` reports the build completed and backend CTest ran 179 tests
with 0 failures.
