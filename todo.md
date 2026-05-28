Status: Active
Source Idea Path: ideas/open/67_aarch64_local_slot_address_offset_probe.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Inventory Caller And Condition Paths

# Current Packet

## Just Finished

Step 1 completed: inventoried caller and condition paths around
`local_slot_address_frame_offset` without implementation edits.

Direct caller inventory:
- `local_slot_address_frame_offset` is declared in
  `dispatch_publication.hpp` and defined in `dispatch_publication.cpp`; `rg`
  found no direct call sites in the repo outside that declaration/definition.
- Adjacent `local_aggregate_address_frame_offset` is also declaration/definition
  only; its implementation delegates to
  `prepared_frame_address_offset_for_value(context, value_name, std::nullopt)`.
- Current local-slot address publication callers do not call
  `local_slot_address_frame_offset`:
  `dispatch.cpp` calls `lower_local_slot_address_publication`, and
  `dispatch_value_materialization.cpp` calls
  `emit_local_slot_address_publication_to_register`.

Current local-slot publication paths and conditions:
- `lower_local_slot_address_publication` accepts only a `bir::BinaryInst` with
  a named result. It uses a prepared result register when available, otherwise
  it requires the result home to be a stack slot with `offset_bytes` plus a
  reserved GPR scratch, then stores the computed pointer back to that result
  stack home.
- `emit_local_slot_address_publication_to_register_impl` is the shared path
  behind both public local-slot publication helpers. It requires pointer
  `Add`/`Sub`, pointer operand type, named LHS, immediate RHS, a valid target
  GPR, and a non-negative adjusted offset.
- The base offset for that path comes from
  `prepared_frame_address_offset_for_value(context, lhs_value_name,
  instruction_index)`, not from `local_slot_address_frame_offset`.
- `prepared_frame_address_offset_for_value` requires prepared module state,
  address-materialization lookups, a control-flow block, valid value name, a
  `PreparedAddressMaterializationKind::FrameSlot` in the current block at or
  before the queried instruction, matching `result_value_name`, `frame_slot_id`,
  non-negative `byte_offset`, an existing frame slot, and an addressable stack
  object.

Adjacent aggregate/frame-address comparison:
- The dormant aggregate helper points at the same prepared frame-address offset
  authority, but with no instruction-index bound.
- Live local aggregate call-argument lowering does not use
  `local_aggregate_address_frame_offset`; it requires
  `allows_local_aggregate_address_publication`, pointer argument type, prepared
  GPR result register, and complete
  `LocalFrameAddressMaterialization` source selection. The selected source
  supplies `address_materialization_frame_slot_id`/`source_slot_id` and
  `address_materialization_byte_offset`/`source_stack_offset_bytes`, then
  prints an `add` from `sp`/`x29`.
- Existing tests cover explicit frame-slot address arguments, missing local
  aggregate address fallback, direct and zero-offset local aggregate address
  call arguments, incomplete local frame address selection fail-closed behavior,
  and materialized pointer-addressed stores.

Likely reachability classification:
- Current static call evidence points to `local_slot_address_frame_offset`
  being unreached by existing C++ callers, while nearby live behavior is routed
  through prepared frame-address materialization facts and call source
  selection. This is not yet a dead/live/disabled classification; Step 2 should
  add a narrow runtime probe before any deletion, implementation, or contract
  decision.

## Suggested Next

Step 2 - Build A Narrow Reachability Probe: add the smallest focused test that
constructs a local-slot pointer `Add`/`Sub` with prepared frame-address
materialization facts and proves the current caller path either lowers through
`emit_local_slot_address_publication_to_register` /
`lower_local_slot_address_publication` without using
`local_slot_address_frame_offset`, or exposes a hidden runtime dependency on
the null helper. Include an adjacent aggregate/frame-address assertion so the
probe compares local-slot behavior with the already-live
`LocalFrameAddressMaterialization` route.

## Watchouts

Do not classify `local_slot_address_frame_offset` as dead/live/disabled from
static scan alone. Do not remove the null helper, mutate shared frame-address
authority, weaken tests, or bundle unrelated dispatch-family contraction before
Step 2/3 provides runtime evidence.

Probe design notes:
- Favor an evidence-oriented AArch64 backend test over implementation
  instrumentation. The useful input shape is a binary pointer result whose LHS
  has a prepared `FrameSlot` address materialization in the same block and
  whose RHS immediate adjusts the offset.
- Include failure/negative coverage by removing or incompleting the prepared
  frame-address materialization facts and expecting fail-closed behavior, not
  legacy rederivation from a local-slot name.
- Keep the probe focused on reachability; capability implementation or helper
  deletion belongs only after classification.

## Proof

Read-only inventory proof:
- `rg -n "local_slot_address_frame_offset\\(" . -g '!review/**'`
- `rg -n "local_aggregate_address_frame_offset\\(" . -g '!review/**'`
- `rg -n "emit_local_slot_address_publication_to_register\\(" . -g '!review/**'`
- `rg -n "lower_local_slot_address_publication\\(" . -g '!review/**'`
- Targeted reads of `dispatch_publication.cpp`, `dispatch_value_materialization.cpp`,
  `globals.cpp`, `calls.cpp`, prepared frame-address lookup code, and existing
  AArch64 frame/local-address tests.

`git diff --check` passed. No build/test proof was required for this
inventory-only packet.
