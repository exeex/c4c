# Execution State

Status: Active
Source Idea Path: ideas/open/63_complete_phi_legalization_and_parallel_copy_resolution.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory The Current Phi Legalization Route
Plan Review Counter: 1 / 10
# Current Packet

## Just Finished

Completed Step 1 (`Inventory The Current Phi Legalization Route`) by auditing
`src/backend/prealloc/legalize.cpp`, `src/backend/prealloc/prealloc.hpp`, and
`src/backend/prealloc/regalloc.cpp`: phi handling still splits between
select-tree materialization (`materialize_phi`,
`try_materialize_root_phi_block`), slot-backed fallback in `lower_phi_nodes`
that creates `phi.*` locals plus predecessor `StoreLocalInst` writes, and a
prepared `join_transfers` contract that downstream helpers can query in
`prealloc.hpp` but regalloc does not yet consume.

## Suggested Next

Start Step 2.1 by reshaping the prepared phi handoff around typed per-edge
transfer facts instead of legalization-owned recovery paths: keep
`PreparedJoinTransfer` / `PreparedEdgeValueTransfer` as the seed surface, stop
tying authoritative semantics to `SelectMaterialization` versus
`EdgeStoreSlot`, and prepare the contract that Step 3.1 can hand directly to
regalloc.

## Watchouts

- Do not treat one additional phi testcase shape as success.
- Keep typed semantic ids as the public identity boundary.
- Do not silently fold branch/join ownership work from idea 62 back into this
  route.
- `legalize.cpp` currently records phi meaning in two bootstrap-specific forms:
  `materialize_reducible_phi_trees` emits `SelectMaterialization` transfers,
  while `lower_phi_nodes` falls back to `EdgeStoreSlot` transfers plus
  `phi_observation` on local slots.
- `prealloc.hpp` already exposes authoritative lookup helpers over
  `PreparedControlFlowFunction::join_transfers`, so the prepared contract
  surface exists but still reflects select/slot lowering details.
- `regalloc.cpp` still reconstructs phi moves from surviving `bir::PhiInst` in
  `append_phi_move_resolution`, so the current consumer handoff is not the
  prepared control-flow contract.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log 2>&1`
ran for the audit packet and preserved the canonical proof log in
`test_after.log`; the subset remains red in the same four baseline cases from
`test_before.log` (`backend_codegen_route_x86_64_variadic_double_bytes_observe_semantic_bir`,
`backend_codegen_route_x86_64_variadic_pair_second_observe_semantic_bir`,
`backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir`,
and `backend_codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir`).
