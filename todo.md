Status: Active
Source Idea Path: ideas/open/617_scalar_compare_publication.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Locate Prepared Scalar Compare Publication Authority

# Current Packet

## Just Finished

Completed Step 2 (`Locate Prepared Scalar Compare Publication Authority`) as a localization-only packet. The seven fresh rows remain mapped to the same object-route diagnostic from Step 1: `unsupported_scalar_compare_publication: RV64 object route requires prepared scalar compare result homes and materializable operands`.

Concrete prepared/RV64 authority map:

- Producer point: non-terminator BIR `BinaryInst` compare values enter RV64 object emission through `fragment_for_prepared_instruction(...)` in `src/backend/mir/riscv/codegen/object_emission.cpp`; the compare is recorded in the local `compares` map, then direct branch-condition uses are skipped by `terminator_uses_value_as_condition(...)`.
- Carrier point: compare publication relies on `PreparedFunctionLookups::value_homes` via `prepared_value_home_for(...)`; the result must have a GPR home through `gpr_register_number_for_home(...)`, and FP operands must be available through FPR homes or materializable FP immediate operands.
- Consumer point: `fragment_for_prepared_fp_compare_publication(...)` is the object consumer for materialized FP compare result publication. It currently admits only F32/F64 `Eq` and `Ne`, requires a prepared GPR result home, and materializes only FP zero immediates through `rv64_fpr_compare_operand_register(...)`.
- Existing adjacent consumer: `append_rv64_fp_compare_to_register(...)` already knows how to encode F32/F64 `Eq`, `Ne`, `Slt`, `Sgt`, `Sle`, and `Sge`; it is used by floating select and fused floating branch paths, so the missing authority is not compare encoding itself.
- Existing adjacent materialization: FP arithmetic in `prepared_scalar_emit.cpp` already materializes non-zero FP immediates by loading their raw bits into a GPR and moving to an FPR; compare publication has a narrower zero-only operand helper.

Seven-row classification:

- `src/20000731-1.c`: materialized ordered FP compare around `x >= 10.0`; source loses publication as a scalar compare value with a non-zero FP immediate operand, not as a direct terminator branch.
- `src/20011217-1.c`: materialized ordered FP compare from `(y > x--) != 1`; the FP `Sgt` value becomes a scalar source for a later integer compare, so this is source/value publication.
- `src/930603-1.c`: materialized float compare values in an OR chain, including ordered comparisons against non-zero FP constants; source publication/operand materialization, not select ownership.
- `src/990117-1.c`: materialized ordered FP compare returned from `foo`; destination publication to the prepared GPR result home is required before return handling can consume it.
- `src/gofast.c`: small compare helper functions return FP relational results as `int`; `Eq`/`Ne` are already covered by the existing helper, while ordered `>`, `>=`, `<`, and `<=` require the same scalar publication path.
- `src/loop-8.c`: materialized ordered FP compare against zero in loop control; operand zero materialization is already supported, but ordered FP result publication is not.
- `src/strct-pack-1.c`: materialized FP `Ne`/comparison against a non-zero packed-struct double constant; this is the operand-materialization half of the same compare publication gap.

Boundary finding:

- Branch publication remains separate because `terminator_uses_value_as_condition(...)` prevents direct branch-condition compares from entering this diagnostic, and fused floating branches already consume `append_rv64_fp_compare_to_register(...)`.
- Select publication remains separate because floating select consumers already call `append_rv64_fp_compare_to_register(...)` for their predicate and use their own select-result/source publication paths.
- Join-carrier/select-edge publication remains separate; existing carrier-alias/select-edge code is only adjacent when the compare result is being copied across edges, not for these first-owner object compile failures.
- RV64 materializable operands are part of the owned gap only for scalar compare publication operands: zero immediate is already supported; non-zero F32/F64 immediates need the same raw-bits materialization style already used for FP binary operands.

## Suggested Next

Proceed to Step 3 with one bounded implementation packet: extend the RV64 prepared scalar compare publication path, not branch/select lowering. Add or refactor a helper used by `fragment_for_prepared_fp_compare_publication(...)` so it admits F32/F64 `Eq`, `Ne`, `Slt`, `Sgt`, `Sle`, and `Sge`; requires a prepared GPR result home; reuses existing FPR home lookup; and materializes FP immediates by raw bits using a scratch GPR/FPR policy consistent with existing FP binary materialization. Add focused object-emission tests for ordered FP compare publication and non-zero FP immediate operand publication, then probe all seven rows.

## Watchouts

- Keep the Step 3 repair in scalar compare publication. Do not alter branch, select, join-carrier, unsupported-marker, allowlist, expectation, runtime/accounting, or broad failure-map code.
- Do not satisfy this by named source-file checks. The semantic shape is materialized FP compare result publication to a prepared GPR home, plus materializable FP operands.
- Preserve the fail-closed destination rule: if the compare result has no prepared GPR home, it should still reject as scalar compare publication.
- The static recovery docs still say `3` scalar compare publication rows; the accepted current row set for this plan remains the seven logs under `/tmp/c4c_617_step1_probe`.

## Proof

Localization-only packet. Used existing `/tmp/c4c_617_step1_probe` logs, source inspection of the seven rows, and AST-backed lookup with `c4c-clang-tool-ccdb` for the RV64 prepared compare/publication functions. No build or test command was run, and `test_after.log` was not created or modified.
