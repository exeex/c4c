Status: Active
Source Idea Path: ideas/open/136_bir_prealloc_prepared_query_surface_simplification_audit.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Draft Bounded Follow-Up Ideas If Needed

# Current Packet

## Just Finished

Completed `plan.md` Step 4 by creating bounded follow-up ideas only for the
real simplification candidates justified by the Step 3 classification:

- `ideas/open/137_select_chain_public_owner_cleanup.md`: one public owner for
  select-chain dependency queries currently split across `prepared_lookups.hpp`,
  `publication_plans.hpp`, and `select_chain_lookups.cpp`.
- `ideas/open/138_call_plan_lookup_ownership_cleanup.md`: call-domain
  ownership for `PreparedCallPlanLookups`, outgoing-stack indexes,
  prior-preservation indexes, and call-boundary effect lookups while preserving
  `PreparedFunctionLookups` construction convenience.
- `ideas/open/139_addressing_lookup_ownership_cleanup.md`: addressing/frame
  ownership for memory-access, address-materialization, frame-address-offset,
  and global-load lookup caches now exposed by the generic prepared lookup
  facade.
- `ideas/open/140_edge_copy_facade_split.md`: split reusable edge
  publication/source/home/move facts from current-block join parallel-copy and
  typed stack-source routing conveniences that are close to AArch64 lowering
  shape.

Each created idea names bounded files/query groups, a proof route, and
domain-specific reviewer reject signals for overfit, expectation downgrades,
target-local policy leakage, broad rewrites, and local rediscovery.

### Candidates Intentionally Rejected For New Ideas

- Value-home/storage lookup as a whole: reusable shared semantic fact; only
  frame/stack id helper placement was a small subcase and not enough for a
  standalone idea from this audit.
- Current-block entry publication: semantic query with target-local policy
  already in AArch64 dispatch code.
- Same-block producer/materialization facts: useful target-neutral producer
  facts; splitting by file layout alone would be churn.
- Control-flow/join/compare facts outside edge-copy routing: owner boundaries
  are already coherent enough.
- Runtime-helper cleanup: mostly domain-local already; preservation ownership
  is covered by the call-plan lookup follow-up rather than a separate runtime
  helper idea.
- Obsolete/deletion follow-ups: Step 3 found no query group that was no longer
  needed after later cleanup.

## Suggested Next

Execute `plan.md` Step 5: assemble the closure recommendation in `todo.md`.
Use the Step 1-3 inventory/classification plus the Step 4 idea list above to
produce the prepared-query table, ownership map, consumer map, no-new-idea
decisions, follow-up recommendation summary, and recommendation for whether
`prepared_lookups.*` should stay as one facade, split by domain, or shrink by
moving queries to existing owners.

## Watchouts

- The active audit remains analysis-only; do not edit implementation files,
  tests, build files, or expectations under this plan.
- The four new ideas are durable follow-ups, not part of the active runbook
  until a future lifecycle activation chooses one.
- Keep clean/no-new-idea groups as closure notes in `todo.md`; do not create
  extra broad ideas for them.

## Proof

Analysis/lifecycle packet only. No build/test command was required or run, and
no `test_after.log` was created for this packet.
