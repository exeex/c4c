Status: Active
Source Idea Path: ideas/open/90_out_of_ssa_critical_edge_and_parallel_copy_deepening.md
Source Plan Path: plan.md
Current Step ID: 3.2
Current Step Title: Publish Cycle-Breaking And Temporary Carrier Authority
Plan Review Counter: 1 / 6
# Current Packet

## Just Finished

Step 3.2 now publishes explicit cycle-step authority on out-of-SSA move
resolution output instead of relying on move-bundle insertion order:
`PreparedMoveResolution` carries `source_parallel_copy_step_index`,
`append_phi_move_resolution()` threads the published `PreparedParallelCopyStep`
index through save/move lowering, and
`find_prepared_out_of_ssa_parallel_copy_move_for_step()` lets downstream
consumers recover the authoritative cycle save/move/move entry for a prepared
parallel-copy step without reconstructing it from implementation accident.

## Suggested Next

Stay in Step 3.2 and take the remaining temporary-carrier surface directly:
inspect whether any covered cycle-broken or edge-store bundle families still
force consumers to infer `uses_cycle_temp_source` or carrier provenance from
raw move ordering, then publish the smallest remaining helper or prepared-dump
seam before moving on to Step 3.3 coalescing-boundary cleanup.

## Watchouts

- `source_parallel_copy_step_index` is only populated for
  `PreparedMoveAuthorityKind::OutOfSsaParallelCopy`; keep non-parallel-copy
  move families null so the field stays an authority seam rather than generic
  sequencing metadata.
- The new helper fails closed if multiple move-bundle entries claim the same
  published step index. Preserve that uniqueness if later packets widen the
  cycle-breaking route.
- Keep the route target-independent: predecessor-owned, successor-entry, and
  critical-edge phi copies should continue to publish cycle/temp authority
  through prepared bundle data rather than backend-local placement rules.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' 2>&1 | tee test_after.log`
Passed. Backend build and the `^backend_` subset stayed green after publishing
explicit out-of-SSA parallel-copy step indices on move-resolution records and
adding the authoritative move-for-step helper; canonical proof log path is
`test_after.log`.
