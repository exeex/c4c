# Current Packet

Status: Active
Source Idea Path: ideas/open/52_aarch64_calls_prepared_authority_repair.md
Source Plan Path: plan.md
Current Step ID: Step 5
Current Step Title: Repair after-call result publication authority

## Just Finished

Step 5 repaired after-call result publication authority in `calls.cpp`.
`record_call_result_source_register` no longer scans raw after-call move
bundles for semantic result authority; scalar GPR and FPR call-result source
registers are now recorded from `PreparedCallResultPlan` source register,
bank, placement, width, occupied-register, and destination-value facts.

## Suggested Next

Next bounded implementation packet: supervisor review the Step 5 result
publication slice and decide whether the remaining plan work needs another
prepared-authority cleanup or a reviewer pass before lifecycle closure.

## Watchouts

`record_call_result_source_register` intentionally handles only scalar GPR/FPR
result source publication. Vreg/f128 call-result movement remains handled by
the after-call lowering path rather than scalar-state publication. The
delegated broader `^backend_` run still shows the same two pre-existing
failures previously recorded here: `backend_aarch64_instruction_dispatch` and
`backend_codegen_route_aarch64_dynamic_stack_fixed_slot_uses_fp_anchor`.

## Proof

Ran:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`.

Build passed. CTest reported `165/167` passing with the two known broader
backend failures:
`backend_aarch64_instruction_dispatch` and
`backend_codegen_route_aarch64_dynamic_stack_fixed_slot_uses_fp_anchor`.

Proof log: `test_after.log`.
