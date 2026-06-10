# Route 4 Block-Entry Publication Migration

Status: Active
Source Idea: ideas/open/170_route4_block_entry_publication_migration.md

## Purpose

Turn the Route 4 follow-up into bounded execution: migrate the residual
block-entry publication consumer away from older semantic PHI scans and onto
Route 4 BIR publication availability records or a thin BIR-backed facade.

## Goal

Make the selected block-entry publication consumer read
`Route4PublicationAvailabilityIndex` or an equivalent BIR-backed facade for
availability and value-role identity while prepared block-entry helpers remain
available as oracle or fallback until the consumer no longer needs them.

## Core Rule

Route 4 owns publication availability, source-producer identity, and value-role
identity. Do not move publication emission mechanics, storage encoding,
destination homes, publication ordering, stack-source extension, register-view
conversion, or immediate spelling into BIR.

## Read First

- `ideas/open/170_route4_block_entry_publication_migration.md`
- `docs/bir_prealloc_fusion/phase_c_private_cache_contraction.md`
- Route 4 BIR publication availability records and query helpers
- `src/backend/mir/query.hpp`
- Prepared block-entry publication helper declarations and definitions
- Current-block and block-entry publication consumers

## Scope

- Block-entry publication availability.
- Current-block/block-entry source producer identity.
- Block-entry value-role identity.
- Route-index reference validation where it already exists.
- Prepared block-entry publication helpers only after the migrated consumer no
  longer needs them as oracle or fallback.

## Non-Goals

- Do not change publication hook kind, destination home, storage encoding,
  stack-source extension, register-view conversion, immediate spelling,
  emitted storage, scalar publication emission policy, or publication order.
- Do not hide prepared block-entry helpers before the residual consumer has
  migrated.
- Do not replace typed Route 4 records with broad generic BIR scans.
- Do not claim progress by weakening tests, rewriting expectations, or adding
  named-case shortcuts.

## Working Model

- Route 4 publication availability records are the target-neutral source for
  whether a current-block or block-entry publication relationship exists.
- The migrated consumer should query typed Route 4 records or a narrow facade
  backed by typed Route 4 validation.
- Prepared block-entry helpers remain visible during migration so they can
  serve as comparison oracles and compatibility fallback.
- Missing availability and mismatched block/source/value-role cases must fail
  closed.
- Publication emission and target storage decisions remain outside Route 4.

## Execution Rules

- Start with an inventory packet; do not migrate code before selecting the
  residual consumer and proof subset.
- Prefer existing Route 4 query helpers or a thin BIR-backed facade over manual
  structure scans.
- Keep the first migration focused on one block-entry publication consumer.
- Keep prepared helper contraction separate from consumer migration and only do
  it after direct users have been re-scanned.
- For code-changing steps, prove with build or compile proof first, then the
  focused current-block/block-entry oracle and migrated-consumer subset.
- Escalate validation if public MIR query APIs, route-index facade behavior,
  prepared helper declarations, or publication emission paths change.

## Ordered Steps

### Step 1: Inventory Residual Route 4 Consumers

Goal: Identify the residual block-entry publication consumer and decide whether
it should read typed Route 4 records directly or through a thin BIR-backed
facade.

Primary targets:
- `Route4PublicationAvailabilityIndex` and related Route 4 records
- Route 4 MIR query helpers and route-index validation helpers
- `src/backend/mir/query.hpp`
- Prepared block-entry publication helper declarations and definitions
- Current-block/block-entry publication consumers

Actions:
- Inspect direct users of prepared block-entry publication helpers.
- Classify each use as Route 4 semantic availability, publication emission
  policy, target storage/home policy, oracle/fallback, or unrelated helper
  exposure.
- Select one residual block-entry consumer with nearby positive and negative
  coverage.
- Decide whether the selected consumer can use typed Route 4 queries directly
  or needs a narrow facade backed by typed validation.
- Record rejected consumers and why they remain out of scope in `todo.md`.

Completion check:
- `todo.md` names the selected consumer, target files, proof command, and
  positive and missing/negative cases before implementation begins.

### Step 2: Add Or Confirm Route 4 Oracle Coverage

Goal: Ensure Route 4 and prepared answers can be compared for the selected
consumer before the consumer switch.

Primary targets:
- Current-block publication oracle tests
- Block-entry publication oracle tests
- Route-index reference validation tests where applicable
- Existing narrow MIR publication/query subset for the selected consumer

Actions:
- Add or extend oracle tests for available current-block and block-entry
  publication cases used by the selected consumer.
- Cover missing publication availability.
- Cover mismatched block/source/value-role or invalid reference cases where
  the selected query surface can reject them.
- Keep prepared block-entry helpers visible as oracle sources.

Completion check:
- Focused tests prove available and missing Route 4 answers for the selected
  consumer, including block-entry negative coverage.

### Step 3: Migrate The Selected Consumer

Goal: Move the selected consumer to `Route4PublicationAvailabilityIndex` or a
thin BIR-backed facade without changing publication emission or target storage
policy.

Primary targets:
- The selected consumer files from Step 1
- Route 4 query/facade helpers if a narrow adapter is needed
- Route-index reference validation helpers if the consumer uses a facade

Actions:
- Replace direct prepared semantic publication reads with typed Route 4 query
  reads for availability and value-role identity.
- Keep prepared answers only as oracle, fallback, or target/emission source
  where still required.
- Preserve fail-closed behavior for missing availability, invalid references,
  and mismatched source/value-role cases.
- Avoid broad BIR scans and avoid copying publication emission payloads into
  BIR records.

Completion check:
- The selected consumer no longer uses the prepared block-entry semantic helper
  as its primary publication availability source, and the narrow proof subset
  is green.

### Step 4: Contract Only Proven-Private Prepared Surface

Goal: Hide or narrow only prepared block-entry publication helpers that no
longer have public consumers after the selected migration.

Primary targets:
- Prepared block-entry publication helper declarations
- Prepared lookup fields exposed only for the migrated Route 4 semantic group
- Public includes or context projections made unused by the migration

Actions:
- Re-scan direct helper and field consumers before deleting or hiding anything.
- Keep helpers public when current-block, block-entry, oracle, fallback,
  publication emission, or target storage consumers still require them.
- Limit include/context contraction to behavior-preserving changes.
- Do not contract publication mechanics or storage/emission APIs as part of
  Route 4 semantic migration.

Completion check:
- Any contraction is backed by direct consumer evidence and compile proof; no
  prepared helper still needed by remaining public consumers is hidden.

### Step 5: Validate And Handoff

Goal: Prove the migration and leave precise lifecycle state for the next
packet.

Actions:
- Run the delegated narrow proof command for the selected Route 4 consumer.
- Run broader backend validation if public MIR query APIs, route-index facade
  behavior, prepared helper declarations, or publication emission paths changed.
- Update `todo.md` with proof results, residual consumers, and the next
  bounded Route 4 candidate if one remains.

Completion check:
- Build/proof logs are fresh, `todo.md` reflects the current Route 4 state,
  and prepared block-entry publication surfaces are either still public by
  evidence or contracted after a proven consumer migration.
