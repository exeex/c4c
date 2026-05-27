# Current Packet

Status: Active
Source Idea Path: ideas/open/52_aarch64_calls_prepared_authority_repair.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Repair immediate ABI binding and frame-slot argument move authority

## Just Finished

Step 2 replaced the duplicate authority scans for
`lower_before_call_immediate_binding` and
`find_prepared_frame_slot_call_argument_move`. Prepared call-plan lookups now
index immediate call arguments by call position plus ABI index, and prepared
move-bundle lookups now index before-call argument moves by call position plus
ABI index. `calls.cpp` consumes those queries while preserving the current
AArch64 immediate and frame-slot argument staging and emitted instruction
records.

## Suggested Next

Next bounded implementation packet: continue into Step 3 for the scalar
call-argument producer cluster, starting with
`materialize_scalar_call_argument_value` and
`lower_scalar_call_argument_producers`, and replace same-block producer/raw
argument recovery with prepared producer or source authority.

## Watchouts

The Step 2 lookup maps deliberately return `nullptr` for duplicate prepared
facts so lowering does not pick an arbitrary ABI argument or move. The remaining
scalar producer, boundary source, indirect callee, and result publication
clusters are separate packets; do not fold them into the next scalar-producer
slice.

## Proof

Passed:
`cmake --build --preset default && ctest --test-dir build -R '^(backend_aarch64_return_lowering|backend_prealloc_call_boundary_classification|backend_prepare_frame_stack_call_contract)$' --output-on-failure`.

Proof log: `test_after.log`. Regression guard with
`--allow-non-decreasing-passed` compared the matching focused `test_before.log`
and `test_after.log` as `3/3 -> 3/3` with no new failures.

Supervisor also ran broader `^backend_` after the slice; it still has the two
pre-existing failures already visible without this patch:
`backend_aarch64_instruction_dispatch` and
`backend_codegen_route_aarch64_dynamic_stack_fixed_slot_uses_fp_anchor`.
