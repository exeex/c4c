# Execution State

Status: Active
Source Idea Path: ideas/open/63_complete_phi_legalization_and_parallel_copy_resolution.md
Source Plan Path: plan.md
Current Step ID: 2.1
Current Step Title: Publish Typed Phi Edge-Transfer Facts
Plan Review Counter: 0 / 10
# Current Packet

## Just Finished

Completed Step 2.1 (`Publish Typed Phi Edge-Transfer Facts`) by splitting the
prepared phi handoff into semantic join-transfer kind plus carrier detail:
`PreparedJoinTransferKind` now models semantic `PhiEdge` versus `LoopCarry`,
`PreparedJoinTransferCarrierKind` records select-versus-slot carrier detail,
and `legalize.cpp` now publishes those fields for both select-materialized and
slot-backed phi routes without making carrier choice the main consumer
contract.

## Suggested Next

Start Step 2.2 by making parallel-copy resolution authoritative on top of the
new semantic phi-edge contract: represent move ordering and cycle-breaking
explicitly in shared prepare instead of relying on slot-backed fallback or
carrier-specific incidental order.

## Watchouts

- Do not treat one additional phi testcase shape as success.
- Keep typed semantic ids as the public identity boundary.
- Do not silently fold branch/join ownership work from idea 62 back into this
  route.
- `prealloc.hpp` still carries transitional enum aliases plus
  `effective_prepared_join_transfer_carrier_kind(...)` so older manual
  prepared-control-flow fixtures that only rewrite `storage_name` or `kind`
  keep working while Step 2.2 and Step 3.1 migrate to the new surface.
- `PreparedEdgeValueTransfer` still only records per-edge source/destination
  facts plus optional slot storage, so Step 2.2 still needs an authoritative
  ordering/cycle model for multi-move joins.
- `regalloc.cpp` still reconstructs phi moves from surviving `bir::PhiInst` in
  `append_phi_move_resolution`, so the current consumer handoff is not yet the
  prepared control-flow contract.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log 2>&1`
ran for the Step 2.1 packet and preserved the canonical proof log in
`test_after.log`; the subset returned to the same four baseline failures from
`test_before.log` after fixing an intermediate `backend_x86_handoff_boundary`
regression caused by legacy prepared-contract fixtures that rewrote carrier
state without the new `carrier_kind` field. The remaining baseline reds are
`backend_codegen_route_x86_64_variadic_double_bytes_observe_semantic_bir`,
`backend_codegen_route_x86_64_variadic_pair_second_observe_semantic_bir`,
`backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir`,
and `backend_codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir`.
