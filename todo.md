Status: Active
Source Idea Path: ideas/open/02_aarch64_calls_emission_consolidation.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Select One Retained Metadata Authority Leak

# Current Packet

## Just Finished

Step 1 selected exactly one retained metadata authority leak for the next
checkpoint: `aarch64_indirect_byval_argument_size_bytes` in
`calls_byval_aggregates.cpp`.

Selected leak:

- Retained read to remove:
  `CallInst::arg_abi[argument.arg_index]` in
  `aarch64_indirect_byval_argument_size_bytes`.
- Local decision currently rederived from retained ABI metadata: whether a
  frame-slot call argument is an indirect AArch64 byval aggregate passed in
  integer registers, and the byval byte extent used by
  `calls_moves.cpp` before building `make_byval_register_lane_prepared_source`.
- Prepared replacement fact exists: use `prepared_byval_lane_extent_bytes` from
  `calls_moves.cpp`, which is already keyed by
  `PreparedMoveResolution` (`destination_kind == CallArgumentAbi`,
  register/stack-slot storage, `op_kind == Move`,
  reason `"call_arg_byval_aggregate_register_lanes"`),
  `PreparedCallArgumentPlan` destination lane facts, and
  `PreparedValueHome::size_bytes` plus collected prepared byval lane stores.
- Expected deletion path: remove the
  `aarch64_indirect_byval_argument_size_bytes` declaration/definition and
  replace its only live fallback call in the frame-slot-to-register argument
  publication path with `prepared_byval_lane_extent_bytes(context, move,
  *argument, *source_home, call_plan.instruction_index)`.

## Suggested Next

Step 2 should remove the selected
`aarch64_indirect_byval_argument_size_bytes` authority leak and prove the
frame-slot byval register-lane publication path still consumes only prepared
move, argument, and value-home facts.

Focused implementation proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_call_boundary_owner|backend_codegen_route_aarch64_prepared_call_boundary_scalability|backend_codegen_route_aarch64_local_aggregate_address_pointer_copy_publishes_frame_address)$'`

## Watchouts

- Do not close `ideas/open/02_aarch64_calls_emission_consolidation.md` while
  these durable blockers remain.
- The stack byval fallback path is no longer a blocker; do not re-add
  `aarch64_stack_byval_argument_size_bytes`.
- This selection does not cover
  `aarch64_indirect_register_byval_argument`; that remains a separate
  retained `CallInst::arg_abi` shape predicate after the size fallback is
  removed.
- The next packet should delete the helper boundary in `calls.hpp` only for
  `aarch64_indirect_byval_argument_size_bytes`; avoid broad helper cleanup.
- Treat retained `CallInst` reads as acceptable only for identity validation,
  diagnostics, or emission context, not for rederiving prepared call-plan
  decisions.

## Proof

Selection evidence:
`rg -n "arg_abi|arg_types|aarch64_indirect_byval_argument_size_bytes|prepared_byval_lane_extent_bytes|call_arg_byval_aggregate_register_lanes" src/backend/mir/aarch64/codegen/calls*.cpp src/backend/mir/aarch64/codegen/calls.hpp src/backend/prealloc/calls.hpp src/backend/prealloc/regalloc.hpp`

Required packet proof command:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`

Result: passed 162/162 backend tests; proof log preserved at `test_after.log`.
