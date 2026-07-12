# Current Packet

Status: Active
Source Idea Path: ideas/open/717_prepared_mir_join_source_identity_completion.md
Source Plan Path: plan.md
Current Step ID: 2.4
Current Step Title: Review Step 2 boundary coverage

## Just Finished

- Plan Step 2.3.1 now requires exactly one kind-matching typed producer pointer
  at the prepared-MIR and BIR adapters, requires producer/predecessor agreement
  at both availability gates, and keys duplicate/conflict detection by the
  semantic edge plus exact typed destination slot instead of source identity.
- Focused negative coverage now rejects missing, wrong, and contradictory
  producer pointers, producer-block mismatch, exact duplicates, and a
  conflicting source claiming the same semantic destination slot.
- The focused positive fixture now models PHIs in the successor and their
  named producer instructions in the actual predecessor; named, immediate,
  and stack rows remain Available and preserve exact authority across adapters.
- Named incoming-expression evidence now resolves the authoritative producer
  block through the BIR function and fails closed when that exact block or
  instruction cannot be resolved; the named-register row needs no fixture
  bypass.

## Suggested Next

- Execute Plan Step 2.4 review against the corrected Step 2 boundary and its
  fresh focused proof.

## Watchouts

- `source_producer_block_label` is authored from the indexed producer's actual
  BIR block label; focused fixtures must keep producer instructions in the
  predecessor rather than co-locating them with successor PHIs.
- Prepared-MIR join queries must pass their BIR function through to prealloc so
  predecessor-owned evidence is resolved against the complete function.
- Publication, move, bundle, and producer pointers remain opaque identity
  tokens after prepared-core destruction; do not dereference them in review.

## Proof

- Passed: `CMAKE_BUILD_PARALLEL_LEVEL=1 cmake --build --preset default && ctest
  --test-dir build -j --output-on-failure -R '^backend_prepared_lookup_helper$'`.
- Full combined proof output is preserved in `test_after.log`.
