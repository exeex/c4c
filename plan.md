# Route 5 Current-Block Join-Source Migration

Status: Active
Source Idea: ideas/open/171_route5_current_block_join_source_migration.md

## Purpose

Turn the Route 5 follow-up into bounded execution: migrate the residual
current-block join-source identity helper and one selected join-source consumer
away from prior semantic/prepared lookup behavior and onto Route 5 BIR
edge/join-source records.

## Goal

Make `mir::find_bir_current_block_join_source_identity(...)` and a selected
join-source consumer read `Route5EdgeJoinSourceIndex` or a narrow BIR-backed
facade for source/no-source, optional memory-source, and join-source identity,
while prepared current-block join-source helpers remain visible until direct
consumers no longer need them.

## Core Rule

Route 5 owns CFG edge publication identity and current-block join-source
semantic identity. Do not move parallel-copy scheduling, move bundle order,
cycle temporary routing, execution site policy, phase/carrier policy,
coalescing, redundancy, destination registers, storage-sharing checks, or
prepared move records into BIR.

## Read First

- `ideas/open/171_route5_current_block_join_source_migration.md`
- `docs/bir_prealloc_fusion/phase_c_private_cache_contraction.md`
- Route 5 records and indexes:
  `Route5EdgeJoinSourceIndex`,
  `Route5CfgEdgePublicationRecord`,
  `Route5CurrentBlockJoinSourceRecord`,
  `Route5PublicationValueRecord`,
  `route5_build_edge_join_source_index`,
  `route5_find_cfg_edge_publication`, and
  `route5_find_current_block_join_source`
- `src/backend/mir/query.hpp`
- `src/backend/mir/query.cpp`
- Prepared current-block join-source helper declarations and definitions
- AArch64 edge-copy/join/publication consumers

## Scope

- Current-block join-source semantic identity.
- Edge/join value records.
- Source, no-source, and optional memory-source cases.
- Missing predecessor and fail-closed join rejects.
- One selected edge-copy, join, or publication consumer that observes the
  migrated helper.
- Prepared current-block join-source helpers only after the migrated consumers
  no longer need them as public helpers, oracle inputs, or fallback.

## Non-Goals

- Do not change move bundle order, parallel-copy execution planning, cycle
  temporary routing, execution site selection, carrier/phase policy,
  coalescing, redundancy, destination register assignment, storage-sharing
  checks, prepared move records, or AArch64 edge-copy emission policy.
- Do not hide prepared current-block join-source helpers before consumer
  migration proves they are no longer publicly needed.
- Do not claim progress by weakening tests, rewriting expectations, or adding
  named-case shortcuts.
- Do not replace typed Route 5 records with broad generic BIR scans.

## Working Model

- Route 5 BIR records describe edge publication source identity and
  current-block join incoming source identity before target copy scheduling.
- `Route5EdgeJoinSourceIndex` is the target-neutral lookup surface for
  current-block join-source identity.
- Prepared current-block join-source helpers remain available during migration
  as comparison oracles and compatibility fallback where still required.
- Missing predecessor, no-source, memory-source, and mismatched join requests
  must fail closed through the migrated query surface.
- Target/prealloc policy stays downstream of Route 5.

## Execution Rules

- Start with an inventory packet; do not migrate code before selecting the
  residual consumer and proof subset.
- Prefer existing Route 5 query helpers or a thin BIR-backed facade over manual
  structure scans.
- Keep the first migration focused on
  `mir::find_bir_current_block_join_source_identity(...)` and one selected
  join-source consumer.
- Keep prepared helper contraction separate from consumer migration and only do
  it after direct consumers have been re-scanned.
- For code-changing steps, prove with build or compile proof first, then the
  focused edge publication, join-source oracle, and migrated-consumer subset.
- Escalate validation if public MIR query APIs, Route 5 index/facade behavior,
  prepared helper declarations, AArch64 edge-copy/publication paths, or
  publication planning surfaces change.

## Ordered Steps

### Step 1: Inventory Residual Route 5 Consumers

Goal: Identify the residual current-block join-source helper and select the
join-source consumer that should observe the first Route 5 migration.

Primary targets:
- `mir::find_bir_current_block_join_source_identity(...)`
- `Route5EdgeJoinSourceIndex` and related Route 5 records
- Prepared current-block join-source helper declarations and definitions
- AArch64 edge-copy, join, and publication consumers
- Existing Route 5 oracle and prepared lookup helper tests

Actions:
- Inspect direct users of prepared current-block join-source helpers and
  `mir::find_bir_current_block_join_source_identity(...)`.
- Classify each use as Route 5 semantic identity, target/prealloc scheduling
  policy, publication emission policy, oracle/fallback, or unrelated helper
  exposure.
- Select one consumer with nearby positive, no-source, memory-source, and
  missing-predecessor or fail-closed coverage potential.
- Decide whether the selected consumer can read typed Route 5 queries directly
  or needs a narrow BIR-backed facade.
- Record rejected consumers and why they remain out of scope in `todo.md`.

Completion check:
- `todo.md` names the selected consumer, target files, proof command, and
  positive and negative cases before implementation begins.

### Step 2: Add Or Confirm Route 5 Oracle Coverage

Goal: Ensure Route 5 answers can be compared against prepared semantic answers
for the selected helper and consumer before the switch.

Primary targets:
- Edge publication oracle tests
- Current-block join-source oracle tests
- Prepared lookup helper tests
- Existing narrow MIR/AArch64 subset for the selected consumer

Actions:
- Add or extend oracle tests for available current-block join-source cases used
  by the selected consumer.
- Cover no-source and optional memory-source cases.
- Cover missing predecessor and fail-closed join rejects where the selected
  query surface can reject them.
- Keep prepared current-block join-source helpers visible as oracle sources.

Completion check:
- Focused tests prove Route 5 and prepared answers agree for available,
  no-source, memory-source, and missing/fail-closed cases relevant to the
  selected consumer.

### Step 3: Migrate The MIR Join-Source Helper

Goal: Move `mir::find_bir_current_block_join_source_identity(...)` to
`Route5EdgeJoinSourceIndex` or a narrow BIR-backed facade without changing
target copy scheduling or publication emission policy.

Primary targets:
- `src/backend/mir/query.cpp`
- `src/backend/mir/query.hpp` only if the request/result/status shape must
  change
- Route 5 query/facade helpers if a narrow adapter is needed

Actions:
- Replace direct prepared semantic join-source reads with typed Route 5 query
  reads for destination/source identity and source-producer metadata.
- Preserve existing MIR status semantics for available, no-source,
  memory-source, missing predecessor, and fail-closed mismatch cases.
- Keep prepared answers only as oracle, fallback, or target/prealloc source
  where still required.
- Avoid broad BIR scans and avoid copying move scheduling payloads into Route 5
  records.

Completion check:
- The MIR helper no longer uses the prepared current-block join-source helper
  as its primary semantic identity source, and the focused proof subset is
  green.

### Step 4: Migrate The Selected Join-Source Consumer

Goal: Switch the selected consumer from prepared current-block join-source
semantic lookup behavior to the migrated Route 5-backed helper or facade.

Primary targets:
- The selected consumer files from Step 1
- AArch64 edge-copy, join, or publication routing code if selected
- Consumer-specific tests identified in Step 1

Actions:
- Route the selected consumer through the migrated Route 5-backed helper or
  facade.
- Because the selected consumer lives in
  `backend_aarch64_instruction_dispatch`, whose monolithic CTest currently
  fails earlier on unrelated ambient AArch64 debt, do not use that red binary
  as the Step 4 acceptance proof. Add or extract a bounded isolated proof
  harness for
  `current_block_join_query_routing_uses_bir_identity_with_prepared_fallback()`
  or equivalent selected-consumer coverage, and make that harness the
  delegated Step 4 consumer proof.
- The isolated harness must prove the selected consumer observes Route 5
  current-block join-source identity for the available and absent/no-PHI cases
  without weakening or deleting the existing monolithic dispatch coverage.
- Preserve target/prealloc decisions for move order, execution site, carriers,
  redundancy, registers, storage sharing, and prepared move records.
- Preserve fallback behavior only where still needed for target policy outside
  Route 5 semantic identity.
- Prove the selected consumer with the delegated narrow subset:
  `backend_prepared_lookup_helper` plus the new isolated AArch64
  current-block join routing harness.

Completion check:
- The selected consumer observes Route 5 join-source identity for the covered
  semantic cases in a green isolated consumer proof, while target
  scheduling/emission behavior remains unchanged. The existing red
  `backend_aarch64_instruction_dispatch` binary remains a close-level ambient
  debt check unless its failure mode changes to implicate Route 5.

### Step 5: Contract Only Proven-Private Prepared Surface

Goal: Hide or narrow only prepared current-block join-source helpers that no
longer have public consumers after the selected migrations.

Primary targets:
- Prepared current-block join-source helper declarations
- Prepared lookup fields exposed only for the migrated Route 5 semantic group
- Public includes or context projections made unused by the migration

Actions:
- Re-scan direct helper and field consumers before deleting or hiding anything.
- Keep helpers public when AArch64 routing, publication emission, prepared move
  planning, oracle tests, fallback paths, or other production consumers still
  require them.
- Limit include/context contraction to behavior-preserving changes.
- Do not contract target/prealloc scheduling or emission APIs as part of Route
  5 semantic migration.

Completion check:
- Any contraction is backed by direct consumer evidence and compile proof; no
  prepared helper still needed by remaining public consumers is hidden.

### Step 6: Validate And Handoff

Goal: Prove the migration and leave precise lifecycle state for the next
packet or close decision.

Actions:
- Run the delegated narrow proof command for the selected Route 5 helper and
  consumer.
- Run broader backend validation if public MIR query APIs, Route 5
  index/facade behavior, prepared helper declarations, AArch64 edge-copy or
  publication paths, or publication planning surfaces changed. If
  `backend_aarch64_instruction_dispatch` still fails at the pre-existing
  selected f64 global readback case before reaching the selected Route 5
  consumer, record it as ambient close-level debt rather than blocking the
  isolated Step 4 proof.
- Update `todo.md` with proof results, residual consumers, and whether prepared
  current-block join-source surfaces remain public by evidence.

Completion check:
- Build/proof logs are fresh, `todo.md` reflects the current Route 5 state,
  and prepared current-block join-source surfaces are either still public by
  evidence or contracted after a proven consumer migration.
