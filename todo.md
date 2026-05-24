Status: Active
Source Idea Path: ideas/open/prealloc-stack-layout-contract-boundary.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Split Coordinator Object And Slot Phases

# Current Packet

## Just Finished

Step 3 - Split Coordinator Object And Slot Phases completed one narrow
file-local coordinator extraction.

Extracted `plan_function_stack_objects(...)` in
`stack_layout/coordinator.cpp` to own the object-phase sequence inside
`run_stack_layout()`:
- collect function stack objects
- apply alloca and copy coalescing hints
- apply aggregate address-publication hints to object metadata
- summarize inline asm
- apply regalloc hints to the object set

Changed files:
- `src/backend/prealloc/stack_layout/coordinator.cpp`
- `todo.md`

Behavior notes:
- The helper returns the same object vector plus the same inline-asm summary
  that `run_stack_layout()` already needed for its note.
- Slot assignment, frame size/alignment updates, frame-slot map construction,
  and final publication into `prepared_.stack_layout` stay in `run_stack_layout()`.
- Dynamic address materialization/publication remains untouched for Step 4.
- No headers, tests, prepared printer files, slot order, alignment, frame
  sizing, or object authority changed.

## Suggested Next

Step 4 - Separate Address Publication From Orchestration: inspect direct
frame-slot access and address-materialization helpers in
`stack_layout/coordinator.cpp` and decide whether one publication-focused
file-local helper rename or extraction can make the boundary clearer without
moving target emission or changing stack address semantics.

## Watchouts

- `apply_aggregate_address_publication_hints(...)` still belongs to object
  metadata preparation; Step 4 should focus on final address facts and
  `PreparedAddressMaterialization` publication, not this object-phase hint.
- `run_stack_layout()` still owns the slot phase because it threads frame
  sizing, frame alignment, function addressing creation, slot map construction,
  and final prepared-state insertion in one place.
- Keep final target instruction emission outside prealloc.

## Proof

Ran:

```sh
bash -lc 'set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"; } 2>&1 | tee test_after.log'
```

Result: passed, `162/162` backend tests.

Ran `git diff --check`; passed.

Proof log path: `test_after.log`.
