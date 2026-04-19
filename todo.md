# Execution State

Status: Active
Source Idea Path: ideas/open/63_complete_phi_legalization_and_parallel_copy_resolution.md
Source Plan Path: plan.md
Current Step ID: 3.1
Current Step Title: Move Regalloc And Edge-Move Consumers To Prepared Transfers
Plan Review Counter: 3 / 10
# Current Packet

## Just Finished

Completed another Step 3.1 (`Move Regalloc And Edge-Move Consumers To
Prepared Transfers`) slice by tightening the prepare-to-regalloc phi move
contract in `PreparedMoveResolution`. Rather than inventing a fake
`SaveDestinationToTemp` destination record, the packet made cycle-broken phi
moves publish a first-class `uses_cycle_temp_source` flag so immediate
consumers can distinguish temporary-sourced backedge moves from direct
incoming-value moves without parsing the `reason` string. The liveness proof
now checks both the acyclic phi-join path and the cyclic loop-carry path
against that explicit contract bit.

## Suggested Next

Continue Step 3.1 by auditing the next immediate consumers of
`PreparedMoveResolution` and `parallel_copy_bundles` to see whether any path
still infers cycle-temp semantics from `reason` text instead of the new
`uses_cycle_temp_source` field, then widen into Step 3.2 once the
prepare-to-regalloc boundary is contract-strict.

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
- `PreparedMoveResolution` still does not model `SaveDestinationToTemp` as a
  standalone destination record; the authoritative save step remains in
  `parallel_copy_bundles`, while `move_resolution` now carries the
  first-class `uses_cycle_temp_source` bit for the follow-on move that reads
  from that saved temporary.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_prepare_(liveness|phi_materialize)' > test_after.log 2>&1`
ran for this Step 3.1 packet and preserved the canonical proof log in
`test_after.log`; both `backend_prepare_liveness` and
`backend_prepare_phi_materialize` passed with the new first-class
cycle-temp-source contract checks.
