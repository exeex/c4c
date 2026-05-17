Status: Active
Source Idea Path: ideas/open/260_aarch64_float_ops_markdown_shard_implementation_redistribution.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Float Ownership And Boundaries

# Current Packet

## Just Finished

Step 1 of `plan.md` audited the current scalar F32/F64 float ownership and boundaries before code moves.

Move into `float_ops`:
- `alu.cpp`: `scalar_fp_register_view`, `scalar_fp_register_name_with_view`, and `scalar_floating_alu_mnemonic`.
- `alu.cpp`: the scalar FP branch inside `make_scalar_alu_print_lines`, preserving diagnostics for operand count, matching F32/F64 result/operand widths, register-only FP/SIMD operands, and incomplete printable register facts.
- `alu.cpp`: `is_scalar_alu_floating_opcode` and `is_scalar_alu_floating_type`.
- `alu.cpp`: the F32/F64-only classification and `supported_floating_operation` record population inside `make_prepared_scalar_alu_record`, plus the prepared FPR result/operand validation that uses the shared scalar operand helpers.
- `alu.cpp`: `make_prepared_scalar_alu_instruction_record` can remain as the public wrapper but should delegate the F32/F64 body to `float_ops` once the record helper moves.
- `dispatch.cpp`: no separate float body exists today; prepared F32/F64 binary lowering flows through `lower_scalar_instruction`. Step 3 should route that prepared binary FP case through `float_ops` while leaving `dispatch_prepared_block` route ordering intact.

Leave outside `float_ops`:
- `alu.cpp`: integer ALU helpers and lowering, including `is_scalar_alu_integer_opcode`, `integer_scalar_bit_width`, unsigned reduction helpers, post-extension helpers, GP register spelling, fallback scalar lowering, scalar unary construction/printing, and generic `make_scalar_alu_instruction_record`.
- `alu.cpp`: shared prepared scalar lookup/storage helpers such as `find_prepared_scalar_value_home`, `find_prepared_scalar_storage`, `register_class_from_bank`, `make_prepared_scalar_register_operand`, and `make_prepared_scalar_operand`; these are used by scalar ALU, unary, cast/intrinsic-adjacent paths, and i128 helpers.
- `instruction.hpp` / `instruction.cpp`: neutral enums, record structs, machine opcode/name mapping, `scalar_selection_status`, error names, effect extraction, and `make_scalar_instruction`; these remain family-neutral scalar/machine-node core.
- `dispatch.cpp`: `classify_instruction`, generic scalar dispatch sequencing, `lower_scalar_cast_instruction`, F128 helper routing (`lower_f128_runtime_helper_instruction`), i128 routing, intrinsic lowering, and diagnostics assembly.
- `machine_printer.cpp`: `print_scalar` routing, `print_scalar_cast_instruction`, `floating_register_view` as shared printer utility while scalar FP unary intrinsic still uses it, and all F128 helper/cast/intrinsic printer code.
- `src/backend/mir/aarch64/codegen/float_ops.md`: delete only after compiled owner reconciliation in Step 5.

Focused coverage found:
- `backend_aarch64_scalar_record_contract_test`: `supported_and_deferred_scalar_vocabulary_is_explicit` covers public FP opcode/type predicates and deferred non-support boundaries.
- `backend_aarch64_prepared_scalar_alu_records_test`: `supported_floating_scalar_alu_records_preserve_fpr_facts` covers F64 Add/Sub/Mul/SDiv-to-Div and F32 S-register facts; `unsupported_and_incomplete_facts_fail_closed` covers F128 rejection, wrong-bank F64 rejection, compare rejection, and missing/incomplete scalar facts.
- `backend_aarch64_instruction_dispatch_test`: `block_dispatch_lowers_prepared_f64_scalar_alu_with_fpr_registers` covers dispatch-selected prepared F64 scalar ALU; `block_dispatch_defers_floating_scalar_alu_missing_fpr_facts` covers fail-closed missing FPR facts.
- `backend_aarch64_machine_printer_test`: `selected_fp_arithmetic_prints_from_structured_operands` covers `fadd`, `fsub`, `fmul`, and `fdiv` spelling for F32/F64 structured operands.
- Boundary tests to keep green but not treat as float-ops ownership: scalar cast conversion tests, scalar FP unary fabs intrinsic tests, and F128 runtime helper/cast tests.

## Suggested Next

Execute Step 2: create `src/backend/mir/aarch64/codegen/float_ops.hpp` and `float_ops.cpp`, then move only the scalar F32/F64 ALU predicates, mnemonic spelling, prepared-record construction body, and scalar FP print-line helper out of `alu.cpp`.

## Watchouts

- Keep this behavior-preserving; do not expand FP semantics or weaken tests.
- Keep cast, F128, i128, integer ALU, scalar unary, scalar FP unary intrinsic, and shared prepared scalar/core machine-node helpers outside `float_ops`.
- Preserve fail-closed behavior for missing FPR facts, wrong register banks, unsupported opcodes, and F128 operands.
- `ScalarAluOperationKind::Div` is shared: for scalar FP it maps from both `SDiv` and `UDiv` to `fdiv`, while integer divide remains outside the selected integer subset except unsigned power-of-two reductions.
- The first Step 2 move should prefer delegation over signature churn so existing callers and tests continue to include the same public declarations until the new header boundary is stable.

## Proof

Audit-only packet; no build required and no `test_after.log` was produced.

First Step 2 proof subset before code edits:

```bash
{ cmake --build build --target c4c_backend backend_aarch64_scalar_record_contract_test backend_aarch64_prepared_scalar_alu_records_test && ctest --test-dir build -R '^(backend_aarch64_scalar_record_contract|backend_aarch64_prepared_scalar_alu_records)$' --output-on-failure; } > test_after.log 2>&1
```
