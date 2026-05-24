Status: Active
Source Idea Path: ideas/open/prealloc-regalloc-coordinator-contraction.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Contract One Clear Coordinator Boundary

# Current Packet

## Just Finished

Completed Step 2 from `plan.md`: moved the file-local
`expire_completed_assignments(...)` helper from
`src/backend/prealloc/regalloc.cpp` into the existing assignment helper family.

Changed files:
- `src/backend/prealloc/regalloc/assignment.hpp` now declares the assignment
  expiry helper beside `ActiveRegisterAssignment`.
- `src/backend/prealloc/regalloc/assignment.cpp` now owns the unchanged erase
  predicate for expired active assignment spans.
- `src/backend/prealloc/regalloc.cpp` now imports
  `regalloc_detail::expire_completed_assignments` and keeps the allocation
  call site unchanged.

The boundary is behavior-preserving because the helper signature keeps the
same explicit inputs: the active-assignment vector, the current interval start
point, and the `preserve_call_boundary_pressure` boolean. The strict
`end_point < start_point` expiry and the call-boundary-sensitive
`end_point == start_point` path are unchanged.

## Suggested Next

Run the next Step 2 packet as a separate stack-slot publication contraction:
audit whether the frame-slot seed values, frame extent/alignment lookup, and
`publish_regalloc_stack_slots(...)` can move behind `regalloc/stack_slots.*`
without changing prepared stack-layout authority. Do not combine that packet
with ABI binding or prepared-printer moves.

## Watchouts

`regalloc.cpp` still owns allocation phase order and mutable pool selection.
The next stack-slot packet touches prepared frame publication, so it should be
validated separately and should not rename public prepared data or printer
labels. No `regalloc.hpp` contract split was needed for assignment expiry.

## Proof

Ran delegated backend proof:
`bash -lc 'set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"; } 2>&1 | tee test_after.log'`

Result: passed, `162/162` backend tests.

Ran `git diff --check`; passed.

Proof log: `test_after.log`.
