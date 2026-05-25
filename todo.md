Status: Active
Source Idea Path: ideas/open/02_aarch64_calls_emission_consolidation.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Select One Retained Metadata Authority Leak

# Current Packet

## Just Finished

Step 1 selected exactly one surviving retained metadata authority leak for the
next implementation packet:

- Selected target: `outgoing_stack_argument_bytes` in `calls_common.cpp`.
- Retained metadata read: `call.arg_abi[argument.arg_index]`, guarded by
  `argument.arg_index >= call.arg_abi.size()`.
- Decision owned by the retained read: outgoing call stack argument extent,
  computed as `outgoing_stack_argument_size_bytes(call.arg_abi[argument.arg_index])`
  and combined with `argument.destination_stack_offset_bytes` to produce the
  outgoing call area size.
- Prepared fact status: `PreparedCallArgumentPlan` currently carries
  `destination_stack_offset_bytes` but no prepared destination stack size or
  extent. `PreparedCallBoundaryEffectEndpoint` has `stack_size_bytes`, but
  `make_argument_destination_endpoint` does not populate it for argument
  moves. This is a precise missing-prepared-fact blocker for deleting the
  retained `CallInst::arg_abi` size read without moving the same authority into
  target-local reconstruction.
- Expected deletion path: add or populate a prepared outgoing argument stack
  extent fact during call planning, consume that prepared extent in
  `outgoing_stack_argument_bytes`, and then remove the `CallInst` parameter and
  `call.arg_abi[argument.arg_index]` read from that helper.

## Suggested Next

Execute Step 2 for the selected `outgoing_stack_argument_bytes` target. First
make the prepared call plan or prepared call-boundary effect carry the outgoing
stack argument extent, then delete the retained `CallInst::arg_abi` read from
`calls_common.cpp`.

## Watchouts

- Do not replace the `arg_abi` read with another local size derivation in
  AArch64 code. The required authority is the prepared outgoing stack argument
  extent.
- `destination_stack_offset_bytes` alone is insufficient because
  `outgoing_stack_argument_bytes` needs the final byte extent of each outgoing
  stack argument to compute the maximum call area.
- Changing outgoing stack sizing triggers the plan's `^backend_` escalation
  rule after the focused proof is green.

## Proof

No build or ctest was required for this todo-only Step 1 selection packet, and
no `test_after.log` was produced.

Proposed Step 2 focused proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_prepare_frame_stack_call_contract$'`

Because the selected target changes outgoing stack sizing authority, Step 2
should also run the plan-required broader escalation after the focused proof:
`ctest --test-dir build -j --output-on-failure -R '^backend_'`
