Status: Active
Source Idea Path: ideas/open/297_rv64_runtime_local_stack_memory_foundation.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Add direct local pointer deref support if still in scope

# Current Packet

## Just Finished

Step 3 registered `backend_rv64_runtime_local_pointer_deref` and kept it on the
direct local frame-slot path. The prepared case has only frame-slot
base-plus-offset memory facts: `x` in a local I32 frame slot, `p` in a local
pointer-width frame slot, and a frame-slot address materialization for `&x`.

rv64 prepared emission now handles pointer-width local frame-slot stores/loads
with `sd`/`ld` and materializes direct local frame-slot pointer values with
`addi <reg>, sp, <offset>`. The path reuses the prepared stack-layout offset
checks and still fails closed when frame-slot metadata, address materialization,
function ownership, bounds, or signed 12-bit immediates are missing or
mismatched.

## Suggested Next

Delegate Step 4 to an executor: register the next narrow local stack-memory
case from the active plan only if its prepared facts stay within direct local
frame-slot base-plus-offset lowering, then repair that specific semantic gap.

## Watchouts

- Step 3 support is intentionally limited to local pointer values backed by
  prepared frame-slot address materializations and pointer-width frame-slot
  accesses. Do not treat this as global, call, dynamic-stack, aggregate ABI, or
  non-local pointer provenance support.
- The rv64 simple path now has separate I32 and pointer-width frame-slot access
  checks; keep future local-memory cases on prepared metadata rather than
  source-shaped matching.

## Proof

Ran:

`bash -o pipefail -c "{ cmake --build --preset default && ctest --test-dir build -R '^backend_rv64_runtime' --output-on-failure; } 2>&1 | tee test_after.log"`

Result: build succeeded; all 16 `backend_rv64_runtime` tests passed, including
`backend_rv64_runtime_local_array` and
`backend_rv64_runtime_local_pointer_deref`.

Proof log: `test_after.log`.
