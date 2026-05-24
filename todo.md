Status: Active
Source Idea Path: ideas/open/prealloc-regalloc-coordinator-contraction.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Contract One Clear Coordinator Boundary

# Current Packet

## Just Finished

Completed Step 2 from `plan.md`: moved the file-local
stack-slot publication helpers from `src/backend/prealloc/regalloc.cpp` into
the existing `regalloc/stack_slots.*` helper family.

Changed files:
- `src/backend/prealloc/regalloc/stack_slots.hpp` now declares the stack-slot
  frame seed helpers and the publication helper beside `allocate_stack_slot`.
- `src/backend/prealloc/regalloc/stack_slots.cpp` now owns
  `next_frame_slot_id(...)`, `function_frame_extent(...)`,
  `function_frame_alignment(...)`, and `publish_regalloc_stack_slots(...)`.
- `src/backend/prealloc/regalloc.cpp` imports those helpers and keeps the
  allocation call sites and phase order unchanged.
- The previously file-local unused `has_frame_slot_id(...)` helper was not
  carried forward because no stack-slot publication path referenced it.

The boundary is behavior-preserving because regalloc still computes and passes
the mutable frame-slot seed state explicitly into `allocate_stack_slot(...)`,
and `run()` still performs publication after allocation and before
spill/reload/move materialization. Prepared stack-layout authority is preserved:
the moved helper only appends regalloc-owned prepared stack objects/frame slots
for assigned stack slots, using the same source-kind strings, object IDs, slot
IDs, offsets, sizes, alignments, and home-slot flags as before.

## Suggested Next

Move to Step 3 with a separate second-boundary packet. Audit stack-slot
publication fallout first, then choose only one next clear helper-family
boundary if one remains stable.

## Watchouts

`regalloc.cpp` still owns allocation phase order, mutable pool selection, and
the final prepared stack-layout frame-size/frame-alignment maxima. Do not
combine any later ABI binding, value-location, or prepared-printer move with
this stack-slot contraction. No `regalloc.hpp` split or printer-label edit was
needed.

## Proof

Ran delegated backend proof:
`bash -lc 'set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"; } 2>&1 | tee test_after.log'`

Result: passed, `162/162` backend tests.

Ran `git diff --check`; passed.

Proof log: `test_after.log`.
