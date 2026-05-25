Status: Active
Source Idea Path: ideas/open/03_dispatch_responsibility_reduction.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Isolate Edge-Copy And Join-Value Handling

# Current Packet

## Just Finished

Step 4 - Isolate Edge-Copy And Join-Value Handling moved predecessor select
parallel-copy source lowering out of `dispatch.cpp` into
`dispatch_edge_copies.cpp`, with
`lower_predecessor_select_parallel_copy_sources` exposed from
`dispatch_edge_copies.hpp`.

Behavior is preserved: `dispatch.cpp` still calls the helper at the branch
terminator routing point, and the same bundle filters, successor lookup,
source/destination home checks, select-source publication lowering, emitted
scalar recording, and empty-on-failure behavior are retained.

## Suggested Next

Continue Step 4 by isolating the next edge-copy or join-value helper cluster
that still lives in `dispatch.cpp`, keeping call-boundary and publication
owners unchanged unless a declaration dependency proves unavoidable.

## Watchouts

The moved helper still accepts the diagnostics parameter for call-site
compatibility, matching the prior unused-parameter behavior. No call-source
files, dispatch publication files, source tests, or expectation tests changed.

## Proof

Passed. Backend proof log is `test_after.log`: 162/162 backend tests passed.

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log 2>&1
```
