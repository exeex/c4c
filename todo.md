# Current Packet

Status: Active
Source Idea Path: ideas/open/52_aarch64_calls_prepared_authority_repair.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Repair scalar call-argument producer materialization authority

## Just Finished

Step 3 replaced the scalar call-argument materialization path in `calls.cpp`
so `materialize_scalar_call_argument_value` and
`lower_scalar_call_argument_producers` consume prepared edge-publication source
producer facts for scalar binary and load-local argument producers instead of
recovering producer/source authority with recursive same-block producer scans.
Direct-global select-chain call arguments still route through the shared
prepared direct-global select-chain query and prepared call-argument plan.

## Suggested Next

Next bounded implementation packet: continue to the next Step 3-adjacent or
Step 4 call authority cluster selected by the supervisor, with special attention
to any remaining scalar call argument paths that still depend on non-prepared
source recovery outside this packet's two target functions.

## Watchouts

The scalar call-argument producer lookup uses prepared value-name producer facts
and validates the prepared block label plus instruction position before
materializing a binary producer. The delegated broader `^backend_` run still
shows the same two pre-existing failures previously recorded here:
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
