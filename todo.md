# Execution State

Status: Active
Source Idea Path: ideas/open/63_complete_phi_legalization_and_parallel_copy_resolution.md
Source Plan Path: plan.md
Current Step ID: 3.1
Current Step Title: Move Regalloc And Edge-Move Consumers To Prepared Transfers
Plan Review Counter: 0 / 10
# Current Packet

## Just Finished

Completed the first Step 3.1 (`Move Regalloc And Edge-Move Consumers To
Prepared Transfers`) slice by rewriting
`src/backend/prealloc/regalloc.cpp::append_phi_move_resolution(...)` to consume
prepared `parallel_copy_bundles` plus `join_transfers` from authoritative
`prepared_.control_flow` instead of reconstructing phi moves from surviving
`bir::PhiInst` shape. The regalloc packet now keys reasons from prepared phi
transfer semantics (`phi_join_*` / `phi_loop_carry_*`) and
`backend_prepare_liveness` asserts the move-resolution contract against the
published parallel-copy data instead of the old incidental select-consumer
route.

## Suggested Next

Continue Step 3.1 by fencing select-materialized phi joins out of
`append_consumer_move_resolution(...)` so regalloc no longer publishes
compare-operand consumer moves for prepared join results, and add a focused
loop-carry or cycle-backed regalloc proof that the shared parallel-copy step
ordering remains the authoritative source.

## Watchouts

- Do not treat one additional phi testcase shape as success.
- Keep typed semantic ids as the public identity boundary.
- Do not silently fold branch/join ownership work from idea 62 back into this
  route.
- `prealloc.hpp` still carries transitional enum aliases plus
  `effective_prepared_join_transfer_carrier_kind(...)` so older manual
  prepared-control-flow fixtures that only rewrite `storage_name` or `kind`
  keep working while Step 3.1 migrates consumers to the new surface.
- The new cycle planner keys off authoritative published `bir::Value` names,
  not incidental vector order or later name-table interning.
- `append_consumer_move_resolution(...)` still walks legalized instruction
  consumers, so select-materialized phi joins can publish extra select-operand
  consumer records until that surface is fenced to the prepared join-transfer
  contract.
- `PreparedMoveResolution` still cannot encode explicit
  `SaveDestinationToTemp` steps, so cyclic bundles currently surface their
  move-membership and `*_cycle_temp_*` move reasons without a first-class temp
  record.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log 2>&1`
ran for this Step 3.1 packet and preserved the canonical proof log in
`test_after.log`; `backend_prepare_liveness` and `backend_prepare_phi_materialize`
stayed green and the subset returned to the same four baseline failures already
present in `test_before.log`. The remaining baseline reds are
`backend_codegen_route_x86_64_variadic_double_bytes_observe_semantic_bir`,
`backend_codegen_route_x86_64_variadic_pair_second_observe_semantic_bir`,
`backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir`,
and `backend_codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir`.
