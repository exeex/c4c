Status: Active
Source Idea Path: ideas/open/183_phase_e_route5_edge_join_source_view_consumer_migration.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Handoff And Closure Readiness

# Current Packet

## Just Finished

Step 5 prepared the Route 5 current-block join-source migration handoff for
supervisor review and lifecycle closure. The selected consumer is the AArch64
current-block join-source reader behind
`build_current_block_join_prepared_query_routing(...)`, consumed by the
instruction dispatch/current-block join prepared-query routing helpers.

The migrated boundary is the Route 5 indexed current-block join-source view:
`build_current_block_join_prepared_query_routing(...)` builds a
`Route5EdgeJoinSourceIndex` when BIR function data is available and queries
`mir::find_bir_current_block_join_source_identity(...)`, which uses the indexed
Route 5 records through `route5_indexed_current_block_join_source_records(...)`.
The route answer supplies semantic incoming-expression/source identity only.

Prepared behavior remains the fallback and oracle surface. If Route 5 indexed
data is absent, incomplete, mismatched, or unavailable, routing falls back to
`prepare_current_block_join_parallel_copy_source_facts(...)` with the existing
prepared value-home and edge-publication lookup construction. The final
coverage checks route/prepared behavior for normal predecessor routing with and
without attached prepared policy, missing predecessor-block behavior,
no-source fallback, load-local stack/memory-source routing, and absent-route
prepared fallback.

The active idea is closure-ready from this executor packet: the selected
consumer now prefers valid Route 5 semantic source identity while preserving
prepared fallback/oracle behavior, and the focused proof passed. This handoff
does not claim broad prepared API deletion, aggregate contraction, or removal
of remaining prepared helper surfaces.

## Suggested Next

Supervisor can request reviewer scrutiny if desired, then hand the active idea
to the plan owner for lifecycle closure.

## Watchouts

- The Route 5 boundary provides semantic source identity only; do not move
  prepared move-bundle, source/destination home, scheduling, or final edge-copy
  policy into BIR.
- The indexed MIR query intentionally treats missing indexed publication data as
  unavailable so `build_current_block_join_prepared_query_routing(...)` can fall
  back to prepared facts.
- Prepared value-home lookup construction, prepared edge-publication lookup
  construction, current-block join parallel-copy source facts, move-bundle
  policy, source/destination homes, scheduling, branch spelling, and final
  edge-copy records remain intentional non-goals for this route.
- The missing predecessor-block fixture documents current selected-reader
  behavior: the current-block source identity remains available because this
  reader reasons from the join block source expression, not from predecessor
  edge-copy policy.
- Do not treat this closure as permission for broad prepared API deletion or
  aggregate contraction; those require separate bounded plans.

## Proof

No build/tests were run for Step 5 because this packet is a `todo.md` handoff
summary only.

Final focused proof already accepted for the completed migration:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_current_block_join_routing|backend_prepared_lookup_helper|backend_aarch64_instruction_dispatch)$'; } > test_after.log 2>&1`

Result: passed. The supervisor rolled the latest accepted `test_after.log`
forward to `test_before.log` after accepting Step 4, so `test_before.log`
contains the current focused proof baseline.
