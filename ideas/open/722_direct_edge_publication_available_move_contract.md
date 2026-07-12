# Direct-Edge Publication Available-Move Contract

Status: Open
Type: common prepared-MIR producer-contract repair
Discovered by: `ideas/open/708_x86_named_handoff_materializer_cleanup.md`
Unblocks: `ideas/open/708_x86_named_handoff_materializer_cleanup.md`

## Goal

Make supported direct-edge register, immediate, and memory publication moves
reach `PreparedMirFunctionView::current_block_direct_edge_publication_sources`
as coherent `Available` typed authority, while incomplete or contradictory
producer evidence remains fail closed.

## Why This Exists

Idea 708 Step 3 removed x86 Route 5 agreement and compatibility fallback, but
the existing register-source shared-publication boundary fixture still
produces no available prepared move intent. Its home-only fixture mutation is
not a producer contract: availability also requires matching publication,
move, source identity, and freshness authority. This gap belongs at the common
producer/query boundary and must not be reconstructed by an x86 consumer.

## In Scope

- Localize the first missing or rejected fact between prepared edge-publication
  production and `current_block_direct_edge_publication_sources`.
- Repair the general producer/admission contract for supported direct-edge
  register sources, with nearby immediate and memory-source coverage.
- Preserve exact predecessor, successor, destination, source, move, producer,
  storage, publication, and freshness identity through the typed view.
- Add focused positive and fail-closed proof for missing, stale, ambiguous,
  mismatched, unsupported, or incomplete authority.
- Run a supervisor-selected broader matching backend comparison before close.

## Out Of Scope

- X86 instruction emission, Route 5 removal, or resuming idea 708's consumer
  slice.
- Route 3 memory-consumer migration, joined-branch lowering, ABI policy, or
  target-local move scheduling.
- Fixture-only injection of prepared facts or value-home mutation presented as
  genuine producer availability.
- Weakening typed admission, restoring route agreement, or accepting an
  incomplete move as available.

## Acceptance Criteria

- The first producer/query fact that prevents the supported register source
  from becoming `Available` is documented before repair.
- Genuine supported register, immediate, and memory direct-edge publications
  expose coherent typed move authority without target assistance.
- Missing, stale, ambiguous, mismatched, unsupported, and incomplete evidence
  remains precise fail-closed state.
- Focused producer/query proof and a broader matching backend comparison are
  green without expectation changes.

## Reviewer Reject Signals

- A conditional keyed to the shared-publication fixture, block labels,
  destination names, or one known register source.
- Manual fixture injection, home-only mutation, or x86-side synthesis claimed
  as producer-contract progress.
- Marking the failing case unsupported, weakening its expectation, accepting
  incomplete authority, or restoring Route 5 as a fallback.
- Helper renames, status reclassification, diagnostic changes, or expectation
  rewrites claimed as capability repair.
- Broad target-lowering, ABI, Route 3, or joined-branch changes outside the
  common direct-edge publication producer/query boundary.
- Retaining the same no-`Available` move-intent failure behind a new helper or
  abstraction name.
