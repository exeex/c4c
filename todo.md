Status: Active
Source Idea Path: ideas/open/260_aarch64_float_ops_markdown_shard_implementation_redistribution.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Move Scalar FP Printer Spelling

# Current Packet

## Just Finished

Step 4 of `plan.md` verified the scalar F32/F64 binary FP printer boundary is already in the intended shape after Step 3.

Completed work:
- Confirmed `float_ops.cpp` owns `fadd`, `fsub`, `fmul`, and `fdiv` mnemonic spelling through `make_scalar_float_alu_print_lines`.
- Confirmed scalar F32/F64 binary FP printer diagnostics for unsupported or incomplete printable facts are emitted from `float_ops`.
- Confirmed `machine_printer.cpp` remains generic for scalar machine-printer routing: it calls the scalar printer and prefixes the returned diagnostic without owning binary FP spelling.
- Left F128 helper printing and scalar FP unary intrinsic printing outside `float_ops`.
- Avoided code churn because the post-Step-3 code was already in the correct Step 4 shape.

## Suggested Next

Execute Step 5: reconcile or retire `float_ops.md` against the compiled `float_ops` owner without changing the already-owned scalar F32/F64 binary FP lowering and printer boundary.

## Watchouts

- `float_ops.md` should remain until the compiled owner is reconciled in Step 5.
- Keep cast, F128, i128, integer ALU, scalar unary, scalar FP unary intrinsic, and shared prepared scalar/core machine-node helpers outside `float_ops`.
- Step 4 did not require edits to `machine_printer.cpp`; avoid pulling scalar FP unary intrinsic or F128 helper printer code into `float_ops` during Step 5.
- Preserve fail-closed behavior for missing FPR facts, wrong register banks, unsupported opcodes, and F128 operands if the markdown reconciliation proposes cleanup.
- The generic scalar ALU record helpers still expose prepared scalar FP record construction for focused record tests; dispatch lowering ownership now enters through `float_ops`.

## Proof

Step 4 printer-boundary proof passed and was written to `test_after.log`:

```bash
{ cmake --build build --target c4c_backend backend_aarch64_machine_printer_test backend_aarch64_scalar_record_contract_test backend_aarch64_prepared_scalar_alu_records_test && ctest --test-dir build -R '^(backend_aarch64_machine_printer|backend_aarch64_scalar_record_contract|backend_aarch64_prepared_scalar_alu_records)$' --output-on-failure; } > test_after.log 2>&1 && git diff --check
```
