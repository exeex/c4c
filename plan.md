# BIR Block-Entry Publication Identity Runbook

Status: Active
Source Idea: ideas/open/155_bir_block_entry_publication_identity.md

## Purpose

Move semantic block-entry and current-block publication availability into
BIR-owned queries while keeping publication hooks, destination homes, storage
encoding, register views, and emission policy downstream.

## Goal

Add BIR block-entry/current-block publication queries that match prepared
source, value, producer, instruction, index, and producer-kind answers for
entry and same-block availability before switching any consumer reads.

## Core Rule

Keep BIR publication identity semantic. Do not import hook kind, destination
home, storage encoding, stack-source extension policy, register-view
conversion, immediate publication payloads, emitted storage availability, or
scalar publication emission policy.

## Read First

- `ideas/open/155_bir_block_entry_publication_identity.md`
- `docs/bir_prealloc_fusion/phase_a_normalization_candidates.md`
- `src/backend/prealloc/select_chain_lookups.hpp`
- `src/backend/prealloc/select_chain_lookups.cpp`
- `src/backend/prealloc/value_locations.hpp`
- `src/backend/prealloc/value_locations.cpp`
- `src/backend/prealloc/publication_plans.hpp`
- `src/backend/prealloc/publication_plans.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_publication.cpp`
- `src/backend/mir/aarch64/codegen/calls.cpp`

## Current Targets

- BIR block-entry/current-block publication relationship keyed by block label,
  value name, and before-instruction index.
- Source producer identity, produced BIR value/name, producer instruction,
  producer instruction index, and source-producer kind.
- Prepared/BIR query-equivalence proof for available and unavailable
  publication consumption paths before consumer switches.
- Scalar publication planning reads only where the read asks which semantic
  source is available, not how that source should be emitted.

## Non-Goals

- Do not move publication hook kind, destination home, storage encoding,
  stack-source extension policy, register-view conversion, immediate
  publication payloads, emitted storage availability, or scalar publication
  emission policy into BIR.
- Do not rewrite publication ordering, move routing, storage planning, ABI
  placement, MIR emission, or AArch64 register-view conversion.
- Do not downgrade tests, weaken supported-path expectations, or claim
  progress through expectation rewrites.

## Working Model

- Prepared current-block publication queries remain the oracle while BIR-owned
  publication identity is introduced.
- BIR answers only which semantic source/value/producer is available at block
  entry or before a current-block instruction.
- Existing prealloc/MIR code continues to own hook selection, home selection,
  storage encoding, register views, and emission.
- Consumer switches are allowed only after prepared/BIR equivalence proves the
  semantic identity fields for the selected path.

## Execution Rules

- Prefer small implementation packets with matching build/test proof.
- Use `todo.md` for packet progress, inspected evidence, proof commands, and
  closure notes.
- Preserve `test_before.log` and `test_after.log` as canonical matched logs
  when a packet changes code.
- Treat testcase-shaped publication shortcuts, named-case-only fixes,
  publication-policy leakage into BIR, and expectation downgrades as route
  failures.
- Escalate to broader backend validation if publication ordering, storage
  planning, value homes, register views, calls, or dispatch publication
  emission behavior changes.
- Keep source-idea edits unnecessary unless durable intent changes.

## Ordered Steps

### Step 1: Locate Prepared Publication Surface And Evidence Set

Goal: establish current query owners, callers, data contracts, and proof cases
before adding BIR-owned publication state.

Primary target: prepared current-block publication consumption and current
block entry publication lookup code.

Actions:
- Locate definitions and direct callers of
  `PreparedCurrentBlockPublicationConsumption`,
  `find_prepared_current_block_publication_consumption`,
  `find_prepared_current_block_entry_publication`, and current-block entry
  publication lookup helpers.
- Identify the semantic fields each query consumes and returns separately from
  rejected hook, home, storage, register-view, and emission fields.
- Identify representative entry-available, same-block-available,
  unavailable, wrong-value, wrong-block, and before-index cases for
  equivalence proof.
- Record the narrow backend/codegen subset the executor should use for the
  first implementation packet.

Completion check:
- `todo.md` names inspected files/functions, callers, candidate proof cases,
  rejected fields, and the proposed first code-changing packet.

### Step 2: Define BIR Publication Identity Records

Goal: add target-neutral BIR record shapes for block-entry and current-block
publication availability.

Primary target: BIR identity/query types near the existing producer/source
relationship surface.

Actions:
- Define request and result records keyed by stable block label, value name,
  and before-instruction index.
- Represent availability, source producer identity, produced BIR value/name,
  producer instruction, producer instruction index, and source-producer kind.
- Reuse or align with the existing BIR producer/source identity vocabulary
  instead of importing prepared publication-storage or hook vocabulary.
- Exclude hook kind, destination home, storage encoding, stack-source
  extension policy, register views, immediate publication payloads, and
  emitted storage availability.

Completion check:
- The new BIR vocabulary compiles and does not import rejected prepared
  publication, storage, value-home, or register-view authority.

### Step 3: Add BIR-Owned Publication Queries

Goal: expose BIR-owned queries that answer the same semantic subset as the
prepared current-block publication surface.

Primary target: per-function BIR analysis or MIR query implementation.

Actions:
- Populate block-entry/current-block publication identity from BIR block,
  value, producer, and instruction-order facts.
- Add query APIs for current-block publication consumption and current-block
  entry publication source identity.
- Keep the existing prepared queries available as comparison oracles.
- Avoid consumer rewrites in this step except for test-only comparison hooks.

Completion check:
- BIR publication queries compile and can be compared against prepared query
  results without changing broad backend behavior.

### Step 4: Prove Prepared/BIR Publication Equivalence

Goal: demonstrate that BIR publication queries match the prepared oracle for
positive and negative paths.

Primary target: targeted backend/codegen tests, assertions, or dumps.

Actions:
- Add or update proof for entry-available, same-block-available,
  unavailable, wrong-value, wrong-block, and before-index paths.
- Compare availability, source producer identity, produced value/name,
  producer instruction, instruction index, and source-producer kind.
- Capture matched `test_before.log` and `test_after.log` for the smallest
  backend/codegen subset that exercises dispatch publication and
  current-block publication consumers.
- Investigate mismatches as semantic gaps instead of special-casing named
  failing cases.

Completion check:
- Matched proof logs show no regression, and equivalence evidence covers each
  current-block publication path named by the source idea.

### Step 5: Switch One Semantic Publication Consumer If Proven

Goal: move one current-block publication or scalar publication planning read to
the BIR query only after equivalence proof exists.

Primary target: one narrow consumer that only asks which semantic source is
available.

Actions:
- Select a consumer with minimal blast radius and no ownership of hook kind,
  destination homes, storage encoding, register views, immediate payloads, or
  emission policy.
- Switch only identity reads already proven equivalent.
- Keep prepared queries available for comparison and fallback during this
  route.
- Run the delegated narrow subset and escalate validation if dispatch
  publication, calls, or scalar publication planning behavior changes.

Completion check:
- One route-local consumer uses BIR publication identity for proven facts,
  matched proof remains green, and no target publication policy rewrite is
  included.

### Step 6: Acceptance Review And Close Payload

Goal: make closure evidence explicit for supervisor and reviewer handoff.

Actions:
- Summarize the BIR publication vocabulary, query APIs, equivalence proof
  cases, and any consumer switch in `todo.md`.
- Confirm the prepared query oracle remains available.
- Confirm no rejected hook, home, storage, register-view, or emission state
  entered the BIR relationship.
- Confirm matched regression logs exist for the final code-changing scope.

Completion check:
- `todo.md` contains close-ready evidence and proof commands needed for
  lifecycle closure review.
