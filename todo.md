Status: Active
Source Idea Path: ideas/open/02_aarch64_calls_emission_consolidation.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Select One Retained Metadata Authority Leak

# Current Packet

## Just Finished

Step 1 selected one retained metadata authority leak for the next Step 2
implementation packet: `aarch64_register_byval_argument_size_bytes` in
`calls_byval_aggregates.cpp`.

- Selected target from the Step 1 primary targets: byval size predicates in
  `calls_byval_aggregates.cpp`, specifically the small register-lane byval
  size helper.
- Retained metadata read: the helper reopens
  `context.bir_block->insts[instruction_index]` as `bir::CallInst`, indexes
  `call->arg_abi[argument.arg_index]`, then checks `type`, `byval_copy`,
  `sret_pointer`, `passed_in_register`, `passed_on_stack`, `primary_class`, and
  `size_bytes`.
- Decision made by the retained read: whether the argument is an AArch64
  integer-class byval aggregate passed in one or two registers with
  `0 < size_bytes <= 16`, and the exact payload byte size to use for the
  register-lane publication source.
- Prepared replacement fact: `PreparedMoveResolution` already marks this path
  with reason `call_arg_byval_aggregate_register_lanes` and carries the
  destination lane width/occupied registers; `PreparedCallArgumentPlan` mirrors
  destination register placement; `PreparedValueHome::size_bytes` carries the
  aggregate extent. The existing `prepared_byval_lane_extent_bytes(...)` helper
  in `calls_moves.cpp` already consumes those prepared facts without reopening
  `CallInst::arg_abi`.
- Missing-prepared-fact blocker: none for this selected small register-lane
  byval size path.
- Expected deletion path: replace remaining Step 2 callers of
  `aarch64_register_byval_argument_size_bytes` in `lower_before_call_move` with
  `prepared_byval_lane_extent_bytes(...)` where the prepared move and
  source-home facts are already in scope, keep retained BIR only for identity or
  diagnostics, then delete the helper declaration from `calls.hpp` and the
  helper definition from `calls_byval_aggregates.cpp` if no callers remain.

## Suggested Next

Execute Step 2 for the selected target: remove
`aarch64_register_byval_argument_size_bytes` as a call-boundary authority by
using the prepared byval lane extent facts already available in
`lower_before_call_move`.

## Watchouts

Keep the Step 2 slice limited to the small register-lane byval size path. Do
not fold in `aarch64_indirect_byval_argument_size_bytes`,
`aarch64_stack_byval_argument_size_bytes`, or
`aarch64_indirect_register_byval_argument` unless the supervisor explicitly
widens the packet.

Changing byval aggregate lane lowering triggers the plan escalation rule: after
the focused proof is green, Step 2 should also expect a `^backend_` backend
checkpoint before acceptance.

## Proof

No build or ctest was required for this todo-only Step 1 selection packet.

Proposed Step 2 focused proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_aarch64_instruction_dispatch$'`

Step 2 escalation note: because this changes byval aggregate lane lowering,
plan rules require `ctest --test-dir build -j --output-on-failure -R '^backend_'`
after the focused proof, unless the supervisor narrows that acceptance policy.
