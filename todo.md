# Current Packet

Status: Active
Source Idea Path: ideas/open/527_bir_route7_comparison_body_extraction.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Focused Route7 Proof

## Just Finished

Step 3: Repair Includes And Internal Linkage completed as a verification-only
checkpoint.

The Step 2 route7 owner move had already repaired the required private helper
surface and build registration:

- `src/backend/bir/bir_private.hpp` exposes the existing comparison producer
  helper declarations needed by `src/backend/bir/bir_route7_comparison.cpp`.
- `tests/backend/bir/CMakeLists.txt` includes
  `src/backend/bir/bir_route7_comparison.cpp` for the direct-source
  `backend_lir_to_bir_notes_test`.
- `src/backend/CMakeLists.txt` required no manual update because backend BIR
  sources are discovered by the existing `file(GLOB_RECURSE ... bir/*.cpp)`
  registration.

Include/build registration/linkage fallout was verified by the already-passing
Step 2 backend build/test proof and the supervisor-accepted matching
before/after backend regression logs. No implementation, test, or log files
were changed in this Step 3 checkpoint.

## Suggested Next

Supervisor can delegate Step 4 focused route7 proof: comparison condition
indexing, materialized-condition consumer coverage, and link-time backend
coverage for the separated owner plus facade-backed helpers.

## Watchouts

- This checkpoint did not rerun commands or modify logs by packet instruction.
- The Step 4 proof should still keep route-index facade bodies,
  materialized-condition public query helpers, fused-compare public query
  helpers, route7 public declarations, and tests unchanged unless the
  supervisor explicitly delegates a code packet.
- The Step 2 diff kept route7 lookup/validation bodies in `bir.cpp`; only
  record/index construction moved.

## Proof

No new command was required for this todo-only Step 3 checkpoint.

Verification basis from Step 2:

```sh
bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"' > test_after.log 2>&1
```

Result recorded in `todo.md`: passed. `test_after.log` remains the canonical
proof log from the Step 2 code packet.
