# Current Packet

Status: Active
Source Idea Path: ideas/open/52_aarch64_calls_prepared_authority_repair.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Repair call-boundary source and indirect-callee authority

## Just Finished

Step 4 boundary-source subpacket replaced the
`materialize_call_boundary_source_to_destination` prepared-name/BIR-result scan
in `calls.cpp` with a prepared edge-publication source-producer lookup. The
call-boundary materializer now consumes prepared producer payloads for the
source value and does not recover semantic source facts by walking prior BIR
results.

## Suggested Next

Next bounded implementation packet: continue Step 4 on the indirect-callee
authority subpacket, keeping it separate from the completed boundary-source
repair unless the supervisor chooses a different boundary.

## Watchouts

The boundary-source helper still requires an existing prepared source-producer
fact for the source value and validates the prepared block label plus producer
instruction position before asking value-publication lowering to materialize
the source into the destination register. The delegated broader `^backend_` run
still shows the same two pre-existing failures previously recorded here:
`backend_aarch64_instruction_dispatch` and
`backend_codegen_route_aarch64_dynamic_stack_fixed_slot_uses_fp_anchor`.

## Proof

Ran:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`.

Build passed. CTest reported `165/167` passing with the two known broader
backend failures:
`backend_aarch64_instruction_dispatch` and
`backend_codegen_route_aarch64_dynamic_stack_fixed_slot_uses_fp_anchor`.

Proof log: `test_after.log`.
