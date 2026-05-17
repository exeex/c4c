Status: Active
Source Idea Path: ideas/open/260_aarch64_float_ops_markdown_shard_implementation_redistribution.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Move Prepared Dispatch Lowering To Float Ops

# Current Packet

## Just Finished

Step 3 of `plan.md` moved prepared scalar F32/F64 binary lowering through the compiled AArch64 `float_ops` owner while preserving existing dispatch ordering and fail-closed behavior.

Completed work:
- Added `lower_prepared_scalar_float_alu_instruction` in `float_ops` to own prepared scalar F32/F64 binary machine-instruction wrapping.
- Routed `dispatch.cpp` to try the float-ops lowering path before generic scalar cast/ALU lowering.
- Made generic `lower_scalar_instruction` decline prepared scalar FP binary nodes so the FP route is not owned by the generic ALU lowering body.
- Preserved the existing prepared-record construction helpers, selected-node facts, emitted machine instruction shape, and fail-closed behavior for incomplete or mismatched FPR facts.
- Avoided edits to tests, `plan.md`, source ideas, `float_ops.md`, machine-printer code, and instruction definitions.

## Suggested Next

Execute Step 4: move scalar F32/F64 FP printer mnemonic spelling and printer diagnostics from `machine_printer.cpp` into `float_ops` while keeping machine-printer routing generic.

## Watchouts

- `float_ops.md` should remain until the compiled owner is reconciled in Step 5.
- Keep cast, F128, i128, integer ALU, scalar unary, scalar FP unary intrinsic, and shared prepared scalar/core machine-node helpers outside `float_ops`.
- Step 4 owns the printer-side move; do not change `instruction.cpp` or instruction definitions unless the supervisor expands ownership.
- Preserve fail-closed behavior for missing FPR facts, wrong register banks, unsupported opcodes, and F128 operands.
- The generic scalar ALU record helpers still expose prepared scalar FP record construction for focused record tests; dispatch lowering ownership now enters through `float_ops`.

## Proof

Step 3 dispatch route proof passed and was written to `test_after.log`:

```bash
{ cmake --build build --target c4c_backend backend_aarch64_instruction_dispatch_test backend_aarch64_prepared_scalar_alu_records_test && ctest --test-dir build -R '^(backend_aarch64_instruction_dispatch|backend_aarch64_prepared_scalar_alu_records)$' --output-on-failure; } > test_after.log 2>&1
```
