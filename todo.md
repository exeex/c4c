Status: Active
Source Idea Path: ideas/open/259_aarch64_cast_ops_markdown_shard_implementation_redistribution.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Move cast construction and lowering behavior

# Current Packet

## Just Finished

Completed Step 3 by moving scalar cast opcode classification, selected-node
record construction, and prepared scalar cast instruction construction into
`cast_ops.cpp`/`cast_ops.hpp`. `dispatch.cpp` now routes cast instructions
through `lower_scalar_cast_instruction(...)` before falling back to generic
scalar lowering; `instruction.cpp` retains the family-neutral scalar
instruction core and compatibility declarations remain in `instruction.hpp`.

## Suggested Next

Delegate the next packet for Step 3 or review scope: reconcile remaining
cast-family compatibility declarations/callers so public cast construction
users include `cast_ops.hpp` directly where feasible.

## Watchouts

- Preserve scalar cast behavior; this is ownership redistribution only.
- Do not expand cast semantics or rewrite expectations to claim progress.
- F128 runtime helper delegation in `dispatch.cpp` was left behavior-preserving;
  this packet did not move those helper-boundary bodies.
- Keep `cast_ops.md` until its valid durable content has been reconciled.

## Proof

Ran exactly:

```bash
{ cmake --build build --target c4c_backend backend_aarch64_scalar_cast_records_test backend_aarch64_prepared_scalar_cast_records_test && ctest --test-dir build -R '^backend_aarch64_.*cast' --output-on-failure; } > test_after.log 2>&1
```

Result: passed. `test_after.log` contains 2/2 matching cast tests passed.

Supervisor validation: also ran
`cmake --build build --target backend_aarch64_instruction_dispatch_test &&
ctest --test-dir build -R '^backend_aarch64_instruction_dispatch$'
--output-on-failure`; result passed, 1/1.
