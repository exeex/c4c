# Current Packet

Status: Active
Source Idea Path: ideas/open/527_bir_route7_comparison_body_extraction.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Repair Includes And Internal Linkage

## Just Finished

Step 2: Create Route7 Comparison Owner completed.

Created `src/backend/bir/bir_route7_comparison.cpp` and moved only the five
mapped route7 record/index construction bodies into it:

- `route7_comparison_operand_record`
- `route7_comparison_instruction_record`
- `route7_branch_condition_record`
- both `route7_build_comparison_condition_index` overloads

Kept route7 public declarations in `src/backend/bir/bir.hpp`. Kept route-index
facade bodies, materialized-condition public query helpers, and fused-compare
public query helpers in `src/backend/bir/bir.cpp`.

Added private declarations for the existing comparison producer helpers in
`src/backend/bir/bir_private.hpp` so the moved route7 owner can call the helper
bodies that remain in `bir.cpp`.

Updated `tests/backend/bir/CMakeLists.txt` to include the new route7 owner in
the direct-source `backend_lir_to_bir_notes_test` registration. No manual
`src/backend/CMakeLists.txt` change was needed because backend BIR sources are
picked up by the existing `file(GLOB_RECURSE ... bir/*.cpp)` registration.

## Suggested Next

Supervisor can delegate Step 3 to verify include/linkage fallout is fully
settled and decide whether the already-passing build/link proof is sufficient
to advance to focused Step 4 proof.

## Watchouts

- `clang-format` is not available in this environment; formatting was kept
  consistent manually.
- Do not move route-index facade bodies, materialized-condition public query
  helpers, fused-compare public query helpers, route7 public declarations, or
  tests beyond required build registration.
- The route7 lookup/validation bodies still live in `bir.cpp`; this packet
  moved construction only.

## Proof

Ran exactly:

```sh
bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"' > test_after.log 2>&1
```

Result: passed. `test_after.log` is the canonical proof log.
