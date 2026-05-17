Status: Active
Source Idea Path: ideas/open/259_aarch64_cast_ops_markdown_shard_implementation_redistribution.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Move cast construction and lowering behavior

# Current Packet

## Just Finished

Completed Step 3 compatibility-boundary cleanup by removing the cast helper
declarations from `instruction.hpp`; direct cast helper users now include
`cast_ops.hpp`, while `instruction.cpp` keeps an explicit `cast_ops.hpp`
include for family-neutral scalar opcode routing.

## Suggested Next

Delegate the next Step 3 packet or supervisor review scope to decide whether
any remaining cast-family behavior should move, especially the F128 helper
boundary bodies that were intentionally left behavior-preserving.

## Watchouts

- Preserve scalar cast behavior; this is ownership redistribution only.
- Do not expand cast semantics or rewrite expectations to claim progress.
- F128 runtime helper delegation in `dispatch.cpp` remains behavior-preserving;
  this packet only cleaned declaration/include ownership.
- Keep `cast_ops.md` until its valid durable content has been reconciled.

## Proof

Ran exactly:

```bash
{ cmake --build build --target c4c_backend backend_aarch64_scalar_cast_records_test backend_aarch64_prepared_scalar_cast_records_test backend_aarch64_scalar_record_contract_test backend_aarch64_machine_printer_test backend_aarch64_instruction_dispatch_test && ctest --test-dir build -R '^(backend_aarch64_scalar_cast_records|backend_aarch64_prepared_scalar_cast_records|backend_aarch64_scalar_record_contract|backend_aarch64_machine_printer|backend_aarch64_instruction_dispatch)$' --output-on-failure; } > test_after.log 2>&1
```

Result: passed. `test_after.log` contains 5/5 selected tests passed.
