# AArch64 Join Parallel-Copy Prepared Query

Ownership class: shared-authority migration

## Goal

Replace local AArch64 reconstruction of current-block join parallel-copy source
relationships with a shared prepared join-copy query.

## Why This Exists

Idea 59 identified `build_current_block_join_parallel_copy_cache` as a
move-forward candidate. Its current role reconstructs join source relationships
from prepared homes and move bundles inside AArch64 dispatch-producer code,
which should instead be represented by the shared prepared move authority.

## In Scope

- `src/backend/mir/aarch64/codegen/dispatch_producers.cpp`
  `build_current_block_join_parallel_copy_cache`.
- Dispatch hook consumers of current-block join copies.
- Prepared move-bundle and value-home inputs used by the cache.
- The shared prepared join-copy query owner that should expose equivalent
  source relationships.

## Out Of Scope

- Changing branch-fusion sequencing, before-return publication ordering, or
  target copy emission.
- Reworking same-block scalar recursion, edge fallback, or select-chain
  dependency discovery.
- Adding one-off cache entries for a named join testcase.
- Treating this as a broad dispatch-producer deletion without preserving query
  semantics.

## Acceptance Criteria

- Current-block join source relationships are provided by a shared prepared
  query instead of reconstructed locally from raw prepared homes and move
  bundles.
- Existing dispatch hook consumers receive equivalent source facts without
  learning prepared move-bundle internals.
- Shared query semantics remain aligned with existing prepared move authority.
- Focused proof covers current-block joins, incoming expression/source
  relationships, prepared homes, and nearby branch-fusion or before-return
  publication consumers.

## Reviewer Reject Signals

- Local AArch64 code continues reconstructing join source relationships from
  raw prepared homes and move bundles.
- Shared query semantics diverge from existing prepared move authority.
- Proof covers only a single dispatch caller.
- The change modifies branch-fusion sequencing while claiming only query
  migration.
- The old cache authority survives under a renamed helper.
