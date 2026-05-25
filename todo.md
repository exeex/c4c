Status: Active
Source Idea Path: ideas/open/02_aarch64_calls_emission_consolidation.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Select One Retained Metadata Authority Leak

# Current Packet

## Just Finished

Step 1 selected one retained AArch64 byval metadata authority leak for the next
implementation packet.

Selected authority leak:
`src/backend/mir/aarch64/codegen/calls_byval_aggregates.cpp`:
`aarch64_stack_byval_argument_size_bytes` rereads
`CallInst::arg_abi[argument.arg_index]` to decide whether a call argument is a
pointer byval stack argument and to recover `abi.size_bytes`.

Retained reads to remove:

- `call->arg_abi.size()` bounds check as byval-shape authority.
- `call->arg_abi[argument.arg_index]`.
- `abi.type`, `abi.byval_copy`, `abi.sret_pointer`, `abi.passed_on_stack`, and
  `abi.size_bytes` checks inside `aarch64_stack_byval_argument_size_bytes`.

Existing prepared replacement fact:
the only current caller already has the prepared stack/byval move context in
`calls_moves.cpp`: the selected path is gated by prepared call-boundary facts
(`PreparedMoveResolution::destination_kind == CallArgumentAbi`,
`destination_storage_kind == StackSlot`, `is_aarch64_byval_register_lane_move`
is false, and `PreparedCallArgumentPlan::destination_stack_offset_bytes`/the
binding stack-slot destination identify the outgoing stack argument). The
aggregate copy byte count should come from the prepared value home
(`PreparedValueHome::size_bytes`) and the prepared destination size
(`PreparedCallArgumentPlan::destination_stack_size_bytes`) instead of falling
back to retained BIR ABI metadata.

## Suggested Next

Implement Step 2 for the selected leak only: remove
`aarch64_stack_byval_argument_size_bytes` from the aggregate stack
call-argument copy path, require prepared aggregate size authority there, and
delete the helper declaration/definition if no uses remain.

Focused proof command for that implementation packet:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_instruction_dispatch|backend_aarch64_call_boundary_owner)$'`

## Watchouts

- Expected deletion path is narrow: update the `calls_moves.cpp` stack
  aggregate-copy size selection to use prepared size only, then remove
  `aarch64_stack_byval_argument_size_bytes` from `calls.hpp` and
  `calls_byval_aggregates.cpp` if it becomes dead.
- If the implementation discovers a reachable aggregate stack call argument
  with neither `PreparedValueHome::size_bytes` nor
  `PreparedCallArgumentPlan::destination_stack_size_bytes`, stop and report a
  missing prepared aggregate-size fact instead of rederiving from `arg_abi`.
- Do not widen into `aarch64_indirect_byval_argument_size_bytes`,
  `aarch64_indirect_register_byval_argument`, or the local address publication
  helpers in the next implementation packet.

## Proof

Fresh Step 1 analysis baseline:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_')`

Result: passed, `162/162` backend tests in `test_after.log`.
