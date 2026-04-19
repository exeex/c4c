# Execution State

Status: Active
Source Idea Path: ideas/open/63_complete_phi_legalization_and_parallel_copy_resolution.md
Source Plan Path: plan.md
Current Step ID: 3.1
Current Step Title: Move Regalloc And Edge-Move Consumers To Prepared Transfers
Plan Review Counter: 2 / 10
# Current Packet

## Just Finished

Completed another Step 3.1 (`Move Regalloc And Edge-Move Consumers To
Prepared Transfers`) slice by adding a loop-cycle regalloc proof to
`tests/backend/backend_prepare_liveness_test.cpp`. The new
`phi_loop_cycle_move_resolution` fixture asserts that regalloc consumes the
authoritative backedge `parallel_copy_bundles` contract for cyclic loop-carry
phis, including the published `*_cycle_temp_*` move reason, and does not fall
back to generic `consumer_*` reconstruction for the phi destinations.

## Suggested Next

Continue Step 3.1 with a narrow follow-up on whether any immediate
prepare-to-regalloc or adjacent edge-move consumer still needs a first-class
`SaveDestinationToTemp` record in `PreparedMoveResolution`, rather than only
the current `phi_loop_carry_cycle_temp_*` reason marker, before widening into
broader Step 3.2 downstream-consumer work.

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
  `SaveDestinationToTemp` steps, so cyclic bundles still surface their
  move-membership and `*_cycle_temp_*` move reasons without a first-class temp
  record; this packet only proved that regalloc consumes that published
  contract consistently.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log 2>&1`
ran for this Step 3.1 packet and preserved the canonical proof log in
`test_after.log`; `backend_prepare_liveness` stayed green with the new
loop-cycle assertion, `backend_prepare_phi_materialize` remained green, and
the subset returned to the same four baseline failures already present in
`test_before.log`. The remaining baseline reds are
`backend_codegen_route_x86_64_variadic_double_bytes_observe_semantic_bir`,
`backend_codegen_route_x86_64_variadic_pair_second_observe_semantic_bir`,
`backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir`,
and `backend_codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir`.
