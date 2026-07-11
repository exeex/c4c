# BIR Memory And Publication View Extraction

Status: Open
Type: Implementation
Parent: `ideas/open/694_bir_route_index_retirement_umbrella.md`
Handoff:
- `docs/bir_route_index_retirement/research_digest.md`
- `docs/bir_route_index_retirement/ownership_dependencies.md`
- `docs/bir_route_index_retirement/ordered_followup_plan.md`
Queue Order: 3

## Goal

Extract named BIR memory and publication views that separate BIR semantic
publication facts from prepared publication authority and stored route
agreement residue.

## Why This Exists

Route 3 owns memory access identity, Route 4 owns current-block and block-entry
publication availability, and Route 5 mixes real CFG-edge publication facts
with trailing `route5_*` agreement annotations. The handoff requires this work
to stay at the BIR publication boundary and not silently turn route status into
prepared or MIR authority.

## Owned Files

- `src/backend/bir/` memory access, publication, CFG-edge, and named
  publication proof view code.
- BIR/prealloc boundary files only for proof adapters that read named
  publication rows without changing executable prepared authority.
- Focused backend tests for named memory or publication proof contracts.

## First Owning Layer

BIR memory and publication semantic view layer.

## First Consumer Migration

Move a Route 4 block-entry or current-block publication proof reader to
`BirPublicationView` or equivalent named vocabulary before Route 5
current-block join-source agreement moves.

## Proof Surface

Prepared publication or prepared dump proof showing block-entry,
current-block, or CFG-edge publication agreement under named vocabulary,
without changing executable prepared edge publication behavior. Object or
runtime proof is required if prepared publication behavior changes.

## Numbered Route APIs Kept Private Compatibility

- `Route3MemoryAccessIndex`
- `Route4PublicationAvailabilityIndex`
- `Route5EdgeJoinSourceIndex`
- Route 5 CFG-edge route records
- Route 5 current-block join-source records
- `route5_join_source`
- `route5_join_source_status`
- `route5_join_source_agrees`

These may back named BIR memory or publication proof adapters during
migration. They must remain diagnostic or compatibility details, not prepared
edge publication authority, source freshness, move-bundle authority,
value-home authority, stack destination authority, or MIR authority.

## In Scope

- Add named memory and publication view wrappers over existing route builders.
- Migrate the first Route 4 publication proof reader to named vocabulary.
- Keep Route 5 cleanup later unless the packet owns only named publication
  proof and not prepared authority.
- Preserve old route fields as rollback diagnostics while consumers move.

## Out Of Scope

- Changing executable prepared edge publication behavior.
- Stack/frame/value-home authority.
- Route 7 comparison proof.
- Dump vocabulary cleanup before named publication proof exists.

## Acceptance Criteria

- A named memory or publication view owns the first migrated proof reader.
- Route 4 and Route 5 status/agreement rows remain private compatibility or
  diagnostics.
- Prepared authority records are unchanged unless the packet includes stronger
  prepared, object, or runtime proof for that behavior change.
- Route 5 agreement residue is not treated as durable prepared state.

## Reviewer Reject Signals

- Reject any implementation that treats Route 4 status, Route 5 status, or
  Route 5 agreement as executable prepared publication authority.
- Reject combining publication cleanup with stack destination authority or test
  dump policy rewrites.
- Reject moving Route 5 early by hiding `route5_*` rows behind a generic
  adapter without naming publication proof ownership.
- Reject expectation rewrites, unsupported-marker edits, allowlist edits, or
  route-dump-only proof for executable behavior changes.
- Reject retaining the old route-only failure mode behind a renamed
  publication helper.
