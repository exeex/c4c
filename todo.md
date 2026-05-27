# Current Packet

Status: Active
Source Idea Path: ideas/open/52_aarch64_calls_prepared_authority_repair.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Repair call-boundary source and indirect-callee authority

## Just Finished

Step 4 indirect-callee subpacket replaced the indirect callee local-load and
select-chain source recovery in `calls.cpp` with prepared source-producer
authority. Indirect callee materialization now seeds from the prepared callee
value-name/source-producer fact, resolves load-local stored values through a
shared prepared lookup query, and drives CSEL recursion from prepared select
producer facts instead of same-block producer walks or call operand spelling.

## Suggested Next

Next bounded implementation packet: supervisor review or route the remaining
Step 4 cleanup, if any, around prepared call-boundary/indirect-callee authority
without reopening the completed boundary-source and indirect-callee repairs.

## Watchouts

The indirect-callee path now requires an existing prepared source-producer for
the prepared callee value name and validates that the producer is in the
current prepared block before materialization. The delegated broader
`^backend_` run still shows the same two pre-existing failures previously
recorded here: `backend_aarch64_instruction_dispatch` and
`backend_codegen_route_aarch64_dynamic_stack_fixed_slot_uses_fp_anchor`.

## Proof

Ran:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`.

Build passed. CTest reported `165/167` passing with the two known broader
backend failures:
`backend_aarch64_instruction_dispatch` and
`backend_codegen_route_aarch64_dynamic_stack_fixed_slot_uses_fp_anchor`.

Proof log: `test_after.log`.
