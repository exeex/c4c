Status: Active
Source Idea Path: ideas/open/03_dispatch_responsibility_reduction.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Boundary Documentation And Final Validation

# Current Packet

## Just Finished

Step 6 - Boundary Documentation And Final Validation added boundary notes for
the dispatch route.

`src/backend/mir/aarch64/codegen/README.md` now states that `dispatch.cpp` owns
prepared block traversal and instruction/terminator routing while
materialization, publication/value-home updates, producer lookup, edge-copy
handling, and call-boundary mechanics live behind named owner surfaces.
`dispatch.cpp` also has a concise call-routing comment pointing readers at
`calls_dispatch_bridge`.

## Suggested Next

Ask plan-owner to decide whether the active runbook can close or needs any
final lifecycle update.

## Watchouts

No markdown shards were added under the codegen directory. The remaining call
routing is intentionally not moved until a shared prepared call-argument source
fact exists; do not add AArch64-local source selection to force more
consolidation.

## Proof

Passed. Final backend proof log was rolled into `test_before.log`: 162/162
backend tests passed before and after, with no new failures.

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log 2>&1
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed
```
