Status: Active
Source Idea Path: ideas/open/297_rv64_runtime_local_stack_memory_foundation.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Lower narrow frame-slot local loads and stores

# Current Packet

## Just Finished

Step 2 repaired rv64 prepared emission for narrow I32 local frame-slot
loads/stores by resolving each prepared frame-slot access to an actual `sp`
displacement from the prepared stack layout.

`backend_rv64_runtime_local_array` now emits `xs[0]` at `0(sp)` and `xs[1]` at
`4(sp)`, so qemu returns the expected status `7`. Unsupported frame-slot forms
still fail closed when prepared metadata is missing, mismatched to the current
function, outside the frame slot bounds, or outside the signed 12-bit immediate
range.

## Suggested Next

Delegate Step 3 to an executor: inspect `local_pointer_deref.c`, register it
only if it remains within local frame-slot base-plus-offset facts, and reuse
the Step 2 stack-offset resolution path for direct local pointer dereference
support.

## Watchouts

- Keep Step 3 limited to direct local frame-slot pointer provenance. Do not
  expand into globals, calls, dynamic stack, aggregate ABI, or non-local pointer
  provenance.
- The Step 2 helper intentionally requires a prepared frame-slot id and verifies
  the frame slot function, access bounds, and signed 12-bit stack displacement.

## Proof

Ran:

`bash -o pipefail -c "{ cmake --build --preset default && ctest --test-dir build -R '^backend_rv64_runtime' --output-on-failure; } 2>&1 | tee test_after.log"`

Result: build succeeded; all 15 `backend_rv64_runtime` tests passed, including
`backend_rv64_runtime_local_array`.

Proof log: `test_after.log`.
