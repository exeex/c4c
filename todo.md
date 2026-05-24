Status: Active
Source Idea Path: ideas/open/prealloc-stack-layout-contract-boundary.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Separate Address Publication From Orchestration

# Current Packet

## Just Finished

Step 4 - Separate Address Publication From Orchestration completed one narrow
file-local coordinator extraction.

Extracted `publish_function_addressing_facts(...)` in
`stack_layout/coordinator.cpp` to own the prepared addressing publication
cluster after slot assignment:
- build the slot-name to frame-slot lookup
- append direct frame-slot memory-access facts
- append address-materialization facts
- preserve existing missing-fact notes for unresolved materializations

Changed files:
- `src/backend/prealloc/stack_layout/coordinator.cpp`
- `todo.md`

Behavior notes:
- The helper reuses the same direct frame-slot access and
  `PreparedAddressMaterialization` append helpers; only the call boundary moved.
- Slot assignment, frame size/alignment updates, prepared stack-layout object
  and frame-slot insertion, and final target instruction emission stay outside
  this helper.
- Address semantics are unchanged: frame-slot offsets, global/TLS/string/label
  materialization policies, pointer-value handling, and missing-fact notes keep
  their existing helper logic.
- No headers, tests, prepared printer files, target emission, slot order,
  alignment, frame sizing, or object authority changed.

## Suggested Next

Step 5 - Align Prepared Frame Printing And Close: inspect
`prepared_printer/frame.cpp` only for terminology drift from the stack-layout
contract cleanup. If no data-family names changed, record the no-code close
decision and leave prepared frame dump semantics untouched.

## Watchouts

- `apply_aggregate_address_publication_hints(...)` still belongs to object
  metadata preparation; it was intentionally not moved into the new addressing
  publication helper.
- `run_stack_layout()` still owns the slot phase and final prepared stack-layout
  publication because it threads frame sizing, frame alignment, function
  addressing creation, object/frame-slot insertion, and notes in one place.
- Keep final target instruction emission outside prealloc; Step 4 only
  publishes prepared addressing facts.

## Proof

Ran:

```sh
bash -lc 'set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"; } 2>&1 | tee test_after.log'
```

Result: passed, `162/162` backend tests.

Ran `git diff --check`; passed.

Proof log path: `test_after.log`.
