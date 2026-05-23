Status: Active
Source Idea Path: ideas/open/aarch64-codegen-cpp-family-consolidation.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Consolidate Dispatch Publication CPP Boundary

# Current Packet

## Just Finished

Completed `plan.md` Step 2 by merging the dispatch publication common helper
definitions into `src/backend/mir/aarch64/codegen/dispatch_publication.cpp`.
Deleted the obsolete helper `.cpp` and removed its explicit
`src/backend/CMakeLists.txt` source-list entry.

Post-merge scans found no remaining source-list or implementation references to
the removed dispatch publication helper `.cpp`; remaining mentions are
lifecycle/history context or the existing `dispatch.hpp` section label.

## Suggested Next

Execute `plan.md` Step 3 for the calls family: inspect the previously noted
`calls_common.cpp` / `calls_effects.cpp` candidates, choose one thin helper
boundary, merge it into the nearest durable calls implementation, update
`src/backend/CMakeLists.txt`, and run the same backend proof.

## Watchouts

- Keep the source idea stable unless durable intent changes.
- Do not merge large semantic lowering modules just to reduce file count.
- Do not hide target-independent logic in AArch64 during consolidation.
- Keep slices behavior-preserving and reviewable.
- Calls helpers are shared across `calls.cpp`, `calls_moves.cpp`, and
  `calls_printing.cpp`; choose the destination by ownership, not just file
  size.
- Keep substantive call ABI, move, printing, and preservation modules separate
  unless the next packet proves a boundary is only connective glue.

## Proof

Proof command run and passed:

```sh
{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } 2>&1 | tee test_after.log
```

Result: build passed; `^backend_` CTest subset passed 149/149 tests.

Proof log: `test_after.log`.
