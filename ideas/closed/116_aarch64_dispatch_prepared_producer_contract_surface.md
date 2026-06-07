# 116 AArch64 Dispatch Prepared Producer Contract Surface

## Goal

Move or expose the remaining prepared edge-publication, current-block producer,
and join-routing facts that AArch64 dispatch still shapes locally before
edge-copy and publication materialization.

## Why This Exists

Idea 115 Step 3 found that the old dispatch-family cleanup shape is no longer
the live problem. `dispatch_edge_copies.cpp` and `dispatch_producers.cpp`
already consume many prepared helpers, including prepared edge-publication
facts, prepared move bundles, and same-block producer queries. The residue is
that AArch64 still owns the routing shape around edge publication producer
contexts, current-block join/source queries, select-chain producer wrappers,
and register clobber/read checks that should be represented by a shared
prepared contract or made visible enough to review before more dispatch
contraction.

This is a post-idea-47 and post-idea-64 follow-up. It must not reopen the old
repair by redoing predecessor scans or the earlier join-cache migration. It
should address the current contract surface that remains after those repairs.

## In Scope

- `src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp`
  edge-publication producer context helpers, source-producer context helpers,
  source memory/register publication helpers, and
  `lower_predecessor_select_parallel_copy_sources`.
- `src/backend/mir/aarch64/codegen/dispatch_producers.cpp` current-block join
  prepared-query routing, current-block join incoming expression/source
  helpers, same-block/select-chain producer wrappers, global-load select-chain
  checks, and publication register clobber/read checks.
- Shared prepared or BIR/prealloc query files that already own edge
  publication, move-bundle, same-block producer, select-chain, or current-block
  publication facts.
- Dump or test visibility needed to prove that dispatch consumes shared facts
  instead of rediscovering producer/publication state locally.
- `dispatch.cpp` and `dispatch_value_materialization.cpp` only as consumers
  that exercise or validate the new contract surface.

## Out Of Scope

- Mechanical file moves, translation-unit deletion, or line-count contraction
  before the shared contract is explicit.
- Reopening the old `dispatch_edge_copies.cpp` prepared-authority repair from
  idea 47 unless current code proves a new uncovered gap.
- Rebuilding the closed join parallel-copy query from idea 64 under a new
  helper name.
- Changing target-local AArch64 move, load, store, branch, register, or memory
  operand emission policy except where a call site must consume a shared fact.
- Broad dispatch phoenix work or unrelated cleanup in `dispatch.cpp` alone.

## Acceptance Criteria

- AArch64 edge-copy and producer dispatch no longer own target-neutral routing
  facts that can be expressed as prepared edge-publication, move-bundle,
  current-block join, same-block producer, or select-chain contract queries.
- Remaining AArch64 code in the touched files is limited to target-local
  emission, register hazard checks that genuinely need machine registers, and
  glue that consumes prepared facts.
- Any required new shared query has focused coverage or dump visibility showing
  the edge/current-block producer facts it exposes.
- Proof covers at least one edge-publication/parallel-copy path and one
  current-block producer or join-routing path that used to depend on local
  AArch64 routing shape.
- `dispatch.cpp` and `dispatch_value_materialization.cpp` continue to consume
  equivalent prepared facts without gaining new producer-discovery logic.

## Likely Files

- `src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_producers.cpp`
- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp`
- Shared prepared/BIR prealloc query owners under `src/backend/mir/`
- Existing backend dump or route tests that cover prepared publication,
  parallel-copy, same-block producer, select-chain, or current-block join facts

## Proof Route

Start with read-only characterization of the current helpers named by idea 115
Step 3. Add or adjust the narrowest dump/test coverage that exposes the shared
prepared facts before deleting local routing authority. Then run the
supervisor-selected backend/AArch64 subset that includes edge-copy,
publication, and dispatch-route coverage. Full-suite proof is not required
unless the implementation touches shared query semantics broadly enough to
affect non-AArch64 backends.

## Reviewer Reject Signals

- The route adds predecessor rescans, BIR-name matching, same-block named-case
  matching, or select-chain special cases instead of consuming prepared facts.
- The old join-routing or edge-publication authority survives under renamed
  AArch64 helpers while the slice claims shared contract progress.
- The diff weakens supported-path tests, marks a covered route unsupported, or
  claims progress through expectation rewrites.
- The proof covers only one narrow named testcase while nearby edge-copy,
  current-block join, or select-chain publication paths remain unexamined.
- A broad dispatch rewrite, phoenix pass, or file move is mixed into the
  contract migration.
- Target-local AArch64 instruction spelling or register policy is moved into
  shared prepared code.

## Closure Note

Closed after Step 5. AArch64 dispatch now consumes shared prepared facts for
edge-copy materializable producer classification, edge-publication source
consistency, current-block join instruction routing, and direct-global
select-chain dependency lookup. Remaining AArch64-local helpers are limited to
machine-register hazard policy and target emission glue. Focused prepared and
AArch64 backend proof covered edge/publication and current-block routing paths,
and supervisor acceptance included the monotonic narrow guard plus broader
`^backend_` validation.
