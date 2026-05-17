Status: Active
Source Idea Path: ideas/open/260_aarch64_float_ops_markdown_shard_implementation_redistribution.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Create Float Ops Owner And Move Construction Helpers

# Current Packet

## Just Finished

Step 2 of `plan.md` created the compiled AArch64 `float_ops` owner for scalar F32/F64 ALU construction helpers and then hardened its print-helper boundary without widening ownership.

Completed work:
- Added `src/backend/mir/aarch64/codegen/float_ops.hpp` and `src/backend/mir/aarch64/codegen/float_ops.cpp` as the compiled owner for scalar FP ALU construction support.
- Moved scalar F32/F64 ALU predicates, mnemonic selection, print-line construction, and prepared-record ownership out of `alu.cpp` and into `float_ops`.
- Kept the moved helpers scoped to scalar FP binary ALU ownership, leaving integer ALU, scalar unary, F128, cast, i128, dispatch, and machine-printer ownership outside Step 2.
- Hardened `make_scalar_float_alu_print_lines` so it checks `scalar.scalar_alu` has a value and is marked `supported_floating_operation` before dereferencing.
- Preserved the existing fail-closed diagnostic style for unprintable scalar FP nodes and avoided edits to tests, `plan.md`, or source ideas.

## Suggested Next

Execute Step 3: route the prepared F32/F64 scalar binary lowering path through the compiled `float_ops` owner while preserving existing dispatch ordering and fail-closed diagnostics.

## Watchouts

- `dispatch.cpp` was intentionally not touched in Step 2; Step 3 owns that route split.
- `float_ops.md` should remain until the compiled owner is reconciled in Step 5.
- Keep cast, F128, i128, integer ALU, scalar unary, scalar FP unary intrinsic, and shared prepared scalar/core machine-node helpers outside `float_ops`.
- Preserve fail-closed behavior for missing FPR facts, wrong register banks, unsupported opcodes, and F128 operands.
- `ScalarAluOperationKind::Div` remains shared: for scalar FP it maps from both `SDiv` and `UDiv` to `fdiv`, while integer divide remains outside the selected integer subset except unsigned power-of-two reductions.

## Proof

Step 2 boundary-hardening proof passed and was written to `test_after.log`:

```bash
{ cmake --build build --target c4c_backend backend_aarch64_scalar_record_contract_test backend_aarch64_prepared_scalar_alu_records_test && ctest --test-dir build -R '^(backend_aarch64_scalar_record_contract|backend_aarch64_prepared_scalar_alu_records)$' --output-on-failure; } > test_after.log 2>&1
```
