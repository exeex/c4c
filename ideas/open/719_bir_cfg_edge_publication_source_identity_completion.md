# BIR CFG Edge-Publication Source Identity Completion

Status: Open
Type: common BIR CFG identity repair
Parent: `ideas/closed/703_bir_mir_contract_abstraction_umbrella.md`
Unblocks: `ideas/open/717_prepared_mir_join_source_identity_completion.md`

## Goal

Make the legacy BIR CFG edge-publication source query resolve exact typed source
identity from the supplied CFG request independently of prepared publication
facts, then compare that result with prepared identity without a circular
oracle.

## Why This Exists

`prepared_and_bir_cfg_edge_publication_source_identity_match` accepts a
`BirCfgEdgePublicationSourceRequest`, but the legacy
`find_bir_cfg_edge_publication_source_identity` path ignores it and re-adapts
the already-derived prepared facts. The resulting agreement check compares two
views of the same prepared oracle rather than independently resolving BIR CFG
identity. Load, cast, binary, and select rows share this boundary; the first
load-local failure is only the earliest call site.

## In Scope

- Trace the request-taking agreement helper and legacy query overloads to the
  earliest point where the BIR CFG request is discarded.
- Resolve exact predecessor block, successor block, destination value, source
  value, and producer instruction or memory identity from BIR CFG authority.
- Preserve typed load, cast, binary, select, and other supported producer
  shapes without selecting by row order, display name, or prepared facts.
- Reject missing destination, unavailable source, wrong edge, stale producer,
  duplicate producer, and memory-identity mismatch states explicitly.
- Compare the independently resolved BIR CFG result with prepared publication
  identity only after both sides have established their own typed authority.
- Add nearby positive and negative proof across multiple producer shapes and
  more than the first failing row.

## Out Of Scope

- Prepared-MIR current-block join-source preparation or
  `find_bir_current_block_join_source_identity`.
- Prepared-call production, block-entry publication identity, or target
  materialization.
- Route restoration, diagnostic vocabulary, register allocation, or move
  scheduling.
- Treating prepared edge-publication facts as BIR CFG source authority.

## Acceptance Criteria

- The first discarded or incorrectly adapted BIR CFG fact and its owning helper
  are documented before repair.
- The request participates in exact typed predecessor, successor, destination,
  producer, and memory identity resolution for all supported source shapes.
- Prepared/BIR agreement uses two independently established identities; the
  BIR side is not reconstructed from prepared facts.
- Missing, stale, duplicate, mismatched, and unavailable authority remains
  typed and fail closed.
- Focused CFG edge-publication identity tests and a supervisor-selected broader
  backend comparison are green without expectation changes.

## Reviewer Reject Signals

- A load-local, fixture-line, destination-name, producer-name, or assertion-order
  special case instead of a general CFG identity rule covering cast, binary,
  select, and memory-producing rows.
- Continuing to ignore `BirCfgEdgePublicationSourceRequest`, adapting prepared
  facts twice, or retaining the circular oracle behind a renamed helper.
- Selecting predecessor, successor, destination, producer, or memory authority
  by vector position, nearest instruction, display name alone, or target output.
- Downgrading supported rows, weakening missing/unavailable/mismatch checks, or
  rewriting expectations to accept non-independent identity.
- Helper renames, status-only reclassification, or test-only changes claimed as
  capability progress.
- Broad rewrites of prepared-MIR join handling, prepared-call production,
  block-entry publication, target materializers, or move scheduling.
