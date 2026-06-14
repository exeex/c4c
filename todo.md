Status: Active
Source Idea Path: ideas/open/261_phase_f3_x86_route3_loadlocal_publication_fixture_support.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Supported Publication Route Contract

# Current Packet

## Just Finished

Step 1 selected the supported route contract for idea 261. Use the existing
joined-branch compare-join EdgeStoreSlot route in
`backend_x86_handoff_boundary_joined_branch_test.cpp`, not the rejected
addressed local-slot guard `join_transfers` shape. The fixture helper should
adapt one authoritative true/false incoming edge so its `incoming_value` is a
real same-predecessor-block `LoadLocal` result with prepared addressing. That
lets `make_prepared_edge_publication_source_producer_lookups(...)` classify
the source as `PreparedEdgePublicationSourceProducerKind::LoadLocal`, then
`make_prepared_edge_publication_lookups(...)` copies the matching
`PreparedMemoryAccess` into `PreparedEdgePublication::source_memory_access`
without hand-building publication rows.

## Suggested Next

Execute Step 2 by adding the smallest fixture/helper wiring for that contract:
start from the joined-branch EdgeStoreSlot helper surface, add a local slot and
a predecessor-block `LoadLocal` selected lane, keep the existing authoritative
join-transfer and parallel-copy ownership intact, and route the resulting
prepared module through the x86 statement-memory path that already calls
`render_agreed_route3_load_local_statement_memory_operand(...)`.

## Watchouts

- Do not revive the rejected synthetic `join_transfers` route from the
  addressed local-slot guard fixture.
- Do not count legacy no-publication fallback as positive Route 3/prepared
  agreement.
- Short-circuit remains a poor Step 2 base because its real join ownership does
  not currently expose a selected `LoadLocal` publication for the compare/load
  path.
- Loop-countdown remains out of route for this idea because its local-slot
  loads are consumed by loop-carry rendering rather than the statement-memory
  agreement facade.
- Joined-branch local-slot rows must not be carrier rewrites only. The selected
  lane has to be a real source producer before the edge publication, with
  prepared memory access available for that load.
- Route guards for the accepted contract are: one authoritative join transfer,
  valid true/false source lane indices, non-empty edge transfers, matching
  predecessor/successor labels, preserved parallel-copy bundle publication, a
  named `LoadLocal` incoming source value in the predecessor block, and a
  `PreparedMemoryAccess` at that load instruction whose result name matches
  the publication source.

## Proof

Read-only design packet. No build was required. Validation: `git diff --check
-- todo.md`.
