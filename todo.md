Status: Active
Source Idea Path: ideas/open/02_aarch64_calls_emission_consolidation.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Remove The Selected Local Decision

# Current Packet

## Just Finished

Step 2 removed the selected retained ABI authority leak:
`aarch64_indirect_byval_argument_size_bytes`.

Completed work:

- Deleted the helper declaration from `calls.hpp`.
- Deleted the helper definition from `calls_byval_aggregates.cpp`, including
  its retained `CallInst::arg_abi[argument.arg_index]` size rederivation.
- Replaced the only live fallback call in `calls_moves.cpp` with
  a prepared-only indirect extent fact from `prepared_indirect_byval_extent_bytes`
  before `make_byval_register_lane_prepared_source`.
- The replacement route is keyed by prepared target profile, move destination
  kind/storage/op, frame-slot source encoding, source value id, source home
  kind, source register bank, GPR ABI destination bank, and
  `PreparedValueHome::size_bytes > 16`; it does not read
  `CallInst::arg_abi`.

## Suggested Next

Step 3 should perform a boundary closeout for the selected leak: verify no
retained replacement authority was introduced, decide whether any remaining
nearby branch shape needs a separate packet, and hand the slice to the
supervisor for route-quality review or commit selection.

## Watchouts

- Do not close `ideas/open/02_aarch64_calls_emission_consolidation.md` while
  these durable blockers remain.
- The stack byval fallback path is no longer a blocker; do not re-add
  `aarch64_stack_byval_argument_size_bytes`.
- This selection does not cover
  `aarch64_indirect_register_byval_argument`; that remains a separate
  retained `CallInst::arg_abi` shape predicate after the size fallback is
  removed.
- Treat retained `CallInst` reads as acceptable only for identity validation,
  diagnostics, or emission context, not for rederiving prepared call-plan
  decisions.
- The indirect byval fallback intentionally remains separate from
  `prepared_byval_lane_extent_bytes`, which is only for small
  `"call_arg_byval_aggregate_register_lanes"` payload-lane moves.

## Proof

Required packet proof command:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_call_boundary_owner|backend_codegen_route_aarch64_prepared_call_boundary_scalability|backend_codegen_route_aarch64_local_aggregate_address_pointer_copy_publishes_frame_address)$') > test_after.log 2>&1`

Result: passed 3/3 selected tests; proof log preserved at `test_after.log`.
