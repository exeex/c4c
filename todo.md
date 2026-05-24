Status: Active
Source Idea Path: ideas/open/shared-mir-same-block-producer-select-queries.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Rewire AArch64 Users To The Shared Query

# Current Packet

## Just Finished

Step 3: Rewire AArch64 Users To The Shared Query completed as a narrow code
slice. `dispatch_publication.cpp` now calls
`mir::find_same_block_named_producer_record` directly in
`value_publication_may_read_register_index` and consumes the shared producer
record for the policy-free producer pointer/index facts used by recursive
read-register analysis.

The AArch64 compatibility wrapper remains in `dispatch_producers.cpp` for
unmigrated users, and no publication/store/call planning semantics moved.

## Suggested Next

Continue Step 3 with one more narrow consumer migration if the supervisor wants
more AArch64 rewiring before review. The remaining wrapper-heavy users are
producer-index and named-producer lookups in value materialization and store
sources; migrate only policy-free facts and leave frame-slot, store-source,
call-boundary, and publication-planning decisions local.

## Watchouts

- Step 3 is not fully complete yet: compatibility wrappers still have live
  AArch64 consumers.
- `dispatch_producers.cpp` remains the AArch64 compatibility surface for other
  consumers. Avoid deleting wrappers before call, publication, store, and
  branch-fusion users are migrated coherently.
- `dispatch_publication.cpp` no longer calls the AArch64 named-producer or
  producer-index wrappers directly, but its publication policy helpers still
  belong in the target layer.
- The shared traversal is predicate-based; keep target-specific questions such
  as direct-global-load materialization decisions outside `src/backend/mir`.

## Proof

Delegated proof command:

`bash -lc 'set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"; } 2>&1 | tee test_after.log'`

Result: passed. `ctest` reported `155/155` backend tests passed, `0` failed.
Proof log: `test_after.log`.
