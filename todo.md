# Execution State

Status: Active
Source Idea Path: ideas/open/63_complete_phi_legalization_and_parallel_copy_resolution.md
Source Plan Path: plan.md
Current Step ID: 3.1
Current Step Title: Move Regalloc And Edge-Move Consumers To Prepared Transfers
Plan Review Counter: 1 / 10
# Current Packet

## Just Finished

Completed the next Step 3.1 (`Move Regalloc And Edge-Move Consumers To
Prepared Transfers`) slice by threading prepared control-flow metadata into
`src/backend/prealloc/regalloc.cpp::append_consumer_move_resolution(...)` and
fencing authoritative `SelectMaterialization` join results out of generic
consumer move reconstruction. Regalloc now leaves prepared phi join results to
`append_phi_move_resolution(...)`, and `backend_prepare_liveness` asserts that
the prepared join destination no longer accumulates `consumer_*` move records.

## Suggested Next

Continue Step 3.1 with a focused loop-carry or cycle-backed regalloc proof so
the shared `parallel_copy_bundles` ordering, including cycle-temp steps, is
proven to remain the authoritative move-resolution source beyond the
select-materialized join case.

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
- `append_consumer_move_resolution(...)` still owns non-phi binary/cast/select
  consumer moves, so any additional fencing should stay limited to
  authoritative prepared join results instead of weakening generic consumer
  bookkeeping.
- `PreparedMoveResolution` still cannot encode explicit
  `SaveDestinationToTemp` steps, so cyclic bundles currently surface their
  move-membership and `*_cycle_temp_*` move reasons without a first-class temp
  record.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log 2>&1`
ran for this Step 3.1 packet and preserved the canonical proof log in
`test_after.log`; `backend_prepare_liveness` and `backend_prepare_phi_materialize`
stayed green with the new join-fencing assertion, and the subset returned to
the same four baseline failures already present in `test_before.log`. The
remaining baseline reds are
`backend_codegen_route_x86_64_variadic_double_bytes_observe_semantic_bir`,
`backend_codegen_route_x86_64_variadic_pair_second_observe_semantic_bir`,
`backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir`,
and `backend_codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir`.
