Status: Active
Source Idea Path: ideas/open/03_dispatch_responsibility_reduction.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Isolate Edge-Copy And Join-Value Handling

# Current Packet

## Just Finished

Step 4 - Isolate Edge-Copy And Join-Value Handling moved block-entry
edge-copy skip/filter ownership out of `dispatch.cpp` into
`dispatch_edge_copies.cpp`, with
`should_emit_block_entry_edge_copy_move` exposed from
`dispatch_edge_copies.hpp`.

Behavior is preserved: the block-entry loop still skips current-join
publication clobbers, same-register out-of-SSA parallel copies, and
source-memory out-of-SSA parallel copies before recording or pushing the move,
but those decisions now live behind the edge-copy-owned predicate.

## Suggested Next

Continue Step 4 by isolating the next join-value helper cluster that still
lives in `dispatch.cpp`, keeping call-boundary retargeting and publication
helpers unchanged unless an include/declaration dependency proves unavoidable.

## Watchouts

`should_emit_block_entry_edge_copy_move` intentionally delegates the existing
current-join publication clobber query to `dispatch_publication`; this packet
only moved block-entry decision ownership and did not move publication helper
implementation. No call-source files, call-boundary retargeting helpers,
source tests, or expectation tests changed.

## Proof

Passed. Backend proof log is `test_after.log`: 162/162 backend tests passed.

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log 2>&1
```
