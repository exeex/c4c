# BIR CFG Edge Publication Identity Runbook

Status: Active
Source Idea: ideas/open/156_bir_cfg_edge_publication_identity.md

## Purpose

Move the target-neutral CFG edge publication and join-source identity facts
toward BIR ownership while keeping parallel-copy execution, move routing, and
target placement in prealloc/codegen.

## Goal

Add BIR-owned edge/source relationship queries that can match the semantic
parts of prepared edge publication, edge-copy source facts, and current-block
join source facts before any consumer switch.

## Core Rule

Only migrate semantic source identity: predecessor label, successor label,
destination value/name, source value/name/kind, producer block/instruction,
optional memory-access identity, and current-block join incoming source. Do
not import move bundles, parallel-copy scheduling, storage-sharing decisions,
register homes, execution sites, phases, carriers, or prepared move records
into BIR.

## Read First

- `ideas/open/156_bir_cfg_edge_publication_identity.md`
- `docs/bir_prealloc_fusion/phase_a_normalization_candidates.md`
- `src/backend/prealloc/publication_plans.hpp`
- `src/backend/prealloc/publication_plans.cpp`
- `src/backend/prealloc/prepared_lookups.cpp`
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`

## Current Targets

- Prepared oracle surfaces:
  - `PreparedEdgePublication`
  - `PreparedEdgePublicationLookups`
  - `PreparedEdgeCopySourceFacts`
  - `PreparedCurrentBlockJoinParallelCopySourceFact`
  - `PreparedCurrentBlockJoinParallelCopySourceFacts`
- Prepared oracle helpers:
  - `make_prepared_edge_publication_lookups`
  - `find_indexed_prepared_edge_publication_source_producer`
  - `prepare_edge_copy_source_facts`
  - `prepare_block_entry_parallel_copy_edge_source_facts`
  - `prepare_current_block_join_parallel_copy_source_facts`
- BIR relationship surface for CFG edge publication/source identity.
- Edge-copy and publication-source consumers only after equivalence is proven.

## Non-Goals

- Do not change parallel-copy step ordering, cycle temporary routing, move
  execution site or block placement, phase/carrier policy, coalescing or
  redundancy flags, destination register names, source/destination homes,
  storage-sharing checks, or prepared move records.
- Do not rewrite ABI, call-boundary, comparison, stack, regalloc, or target
  instruction selection behavior as part of this idea.
- Do not weaken tests or mark supported paths unsupported to claim progress.
- Do not add named-testcase shortcuts; the BIR query must model the semantic
  relationship generally.

## Working Model

- The prepared publication helpers remain the oracle until the BIR answers
  match for each migrated fact family.
- BIR may own the CFG/value relationship: "on predecessor P to successor S,
  destination D receives semantic source S0, optionally produced by BIR node P0
  and optionally tied to source memory-access identity."
- Prealloc/codegen continues to own how that relationship becomes target moves.

## Execution Rules

- Work in narrow, observable steps. Add or expose one relationship family
  before switching consumers to it.
- For each consumer migration, prove old prepared answers and new BIR answers
  are equivalent first.
- Keep comparison code against the prepared oracle until the current step has
  nearby positive and negative coverage.
- If implementation requires touching parallel-copy planning files, stop and
  escalate to the supervisor for broader validation and route review.
- Use matching before/after backend coverage when the supervisor delegates
  proof. The likely proof area is backend BIR prepared lookup and edge-copy or
  dispatch-publication coverage.

## Ordered Steps

### Step 1: Inspect the prepared edge-source oracle

Goal: define the semantic field boundary before adding BIR-facing state.

Primary target: prepared publication/source helpers and their existing tests.

Actions:

- Inspect `PreparedEdgePublication`, `PreparedEdgeCopySourceFacts`, and
  current-block join source fact fields.
- Classify fields into BIR-owned semantic identity versus prealloc/codegen
  routing policy.
- Identify existing helper-test coverage for normal edge, join-source,
  optional memory-source, and no-source paths.
- Record any missing narrow coverage in `todo.md` before implementing code.

Completion check:

- The executor can name the exact source identity fields to model in BIR and
  the exact prepared fields that must remain outside BIR.

### Step 2: Add the BIR edge publication relationship surface

Goal: expose a BIR-owned query shape keyed by predecessor label, successor
label, and destination value/name.

Primary target: BIR relationship or analysis-cache code adjacent to existing
BIR function/block/value metadata.

Actions:

- Add a target-neutral edge publication/source identity record with only the
  allowed semantic fields.
- Include source value/name/kind and source producer block/instruction identity
  where available.
- Include optional source memory-access identity only as a semantic link, not
  as stack/home/offset routing policy.
- Add construction or extraction logic without switching production consumers.

Completion check:

- The project builds, and tests can query the BIR relationship without changing
  edge-copy execution behavior.

### Step 3: Prove prepared-oracle equivalence for edge sources

Goal: compare BIR edge-source answers against the prepared oracle before any
consumer starts relying on BIR.

Primary target: backend BIR prepared-lookup tests and narrow edge publication
coverage.

Actions:

- Add equivalence checks for normal edge publication source identity.
- Add equivalence checks for no-source and unavailable-source paths.
- Add equivalence checks for optional memory-source identity when the prepared
  oracle exposes one.
- Keep the prepared oracle as the source of truth during this step.

Completion check:

- Narrow backend BIR coverage proves BIR answers match prepared publication
  and `PreparedEdgeCopySourceFacts` answers for normal, memory-source, and
  no-source paths.

### Step 4: Add and prove current-block join-source identity

Goal: cover current-block join incoming expression/source identity without
pulling in current-block routing conveniences.

Primary target: current-block join source helpers and their backend tests.

Actions:

- Add BIR representation or query support for current-block join incoming
  expression/source identity.
- Compare against `PreparedCurrentBlockJoinParallelCopySourceFact(s)`.
- Exclude destination register names, stack-source flags, immediate-source
  move details, and storage-sharing convenience fields.

Completion check:

- BIR join-source answers match prepared join-source facts for positive and
  negative join paths, and no move-routing fields are copied into BIR.

### Step 5: Switch one edge-source consumer at a time

Goal: migrate reads only after BIR/prepared equivalence is proven.

Primary target: edge-copy source discovery and publication-source producer
reads selected by the supervisor packet.

Actions:

- Switch a single source-discovery or publication-source consumer to the BIR
  query.
- Keep parallel-copy planning and move execution downstream in prealloc/codegen.
- Retain fallback or comparison checks where needed until equivalent coverage
  is broad enough to remove them.
- Repeat only after the previous consumer has build and narrow proof.

Completion check:

- The switched consumer reads semantic source identity from BIR while generated
  edge-copy behavior remains unchanged against the prepared oracle tests.

### Step 6: Run the acceptance proof

Goal: demonstrate that the idea's semantic migration is complete enough for
supervisor review.

Primary target: backend edge-copy, prepared lookup, and dispatch publication
coverage chosen by the supervisor.

Actions:

- Run matching before/after backend coverage for edge copies and dispatch
  publication.
- Include normal edge, join-source, optional memory-source, and no-source
  paths.
- Escalate to broader backend validation if parallel-copy planning files,
  target dispatch files, or move execution behavior changed.

Completion check:

- Acceptance proof is green, BIR edge queries match prepared semantic answers,
  and edge-copy consumers retain prealloc/codegen ownership of scheduling and
  execution.

## Reviewer Reject Signals

- BIR records or queries include parallel-copy scheduling, cycle temporaries,
  execution-site placement, phase/carrier policy, coalescing or redundancy
  flags, destination registers, storage-sharing checks, or prepared move
  records.
- The route changes move execution policy while claiming source-identity
  migration.
- Tests are weakened, expectations are rewritten as the main proof, or a
  supported path is downgraded without explicit user approval.
- Proof covers only one named edge-copy shape without nearby join-source,
  memory-source, and no-source coverage.
