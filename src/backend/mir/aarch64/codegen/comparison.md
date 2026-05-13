# AArch64 Comparison Legacy Surface

This artifact preserves the useful production shape from the removed
`comparison.cpp` translation surface. The old file was not active C++; it was a
line-commented mirror of the reference Rust implementation at
`ref/claudes-c-compiler/src/backend/arm/codegen/comparison.rs`.

## Role

The surface implemented the `ArmCodegen` comparison, fused compare-branch, and
select lowering hooks. It connected operand materialization, integer condition
code selection, floating-point compare instruction choice, soft-float `F128`
delegation, result storage, label generation, and register-cache invalidation.

The code assumed surrounding AArch64 helpers already owned operand loading,
integer compare instruction emission, comparison-op to condition-code mapping,
condition-code inversion, result storage from `x0`, and final assembly text
emission.

## Entry Points

- `emit_float_cmp_impl(dest, op, lhs, rhs, ty)`: materializes both operands,
  compares them as `F32` or `F64`, maps the comparison op to an AArch64
  condition code, emits `cset x0, <cond>`, and stores the boolean result.
- `emit_f128_cmp_impl(dest, op, lhs, rhs)`: delegates binary128 comparison to
  the shared soft-float helper.
- `emit_int_cmp_impl(dest, op, lhs, rhs, ty)`: emits the integer compare
  instruction through the shared helper, selects the integer condition code,
  emits `cset x0, <cond>`, and stores the boolean result.
- `emit_fused_cmp_branch_impl(op, lhs, rhs, ty, true_label, false_label)`:
  emits an integer compare and branches to the true or false successor without
  materializing a boolean temporary.
- `emit_select_impl(dest, cond, true_val, false_val, ty)`: materializes the
  false value in `x1`, true value in `x2`, compares the condition against zero,
  selects with `csel x0, x2, x1, ne`, invalidates the accumulator cache, and
  stores the selected value.

## Comparison Behavior

Floating-point comparisons preserved the left operand by moving it from `x0`
to `x1` before materializing the right operand. `F32` used `fmov s0, w1`,
`fmov s1, w0`, and `fcmp s0, s1`; other non-`F128` floating-point values used
`fmov d0, x1`, `fmov d1, x0`, and `fcmp d0, d1`.

The archived float condition mapping was direct:

- equality and inequality used `eq` and `ne`;
- less-than used `mi`;
- less-or-equal used `ls`;
- greater-than used `gt`;
- greater-or-equal used `ge`;
- signed and unsigned IR comparison variants shared the same mapping for
  floating-point comparisons.

Integer comparisons did not encode compare instruction details locally. They
delegated operand ordering, width handling, signedness-sensitive compare shape,
and immediate/register handling to `emit_int_cmp_insn`, then relied on
`arm_int_cond_code(op)` for the final `cset` condition.

`F128` comparisons were explicitly not lowered here. The surface treated them
as shared binary128 soft-float behavior owned by `f128_softfloat::f128_cmp`.

## Fused Branch Behavior

The fused compare-branch path emitted an integer compare, mapped the requested
IR comparison to an AArch64 condition code, inverted that code, and emitted a
branch-around shape:

```asm
b.<inverse> .Lskip
b <true_label>
.Lskip:
b <false_label>
```

The helper invalidated the whole register cache after emitting the branches.
It did not use a direct `b.<cond> true; b false` shape, so rebuild work should
preserve or intentionally replace the old fallthrough and label-generation
contract with proof.

## Select Behavior

The select path always materialized both candidate values before testing the
condition. It loaded the false value first, copied it to `x1`, loaded the true
value, copied it to `x2`, then loaded the condition into `x0` and used
`cmp x0, #0` plus `csel x0, x2, x1, ne`.

After the `csel`, the accumulator register cache was invalidated before
storing to the destination. The type parameter was not used in the archived
surface; all selection moved through the integer register path.

## Dependencies

The removed surface depended on these surrounding concepts:

- IR model: `IrCmpOp`, `Operand`, `Value`, and `IrType`.
- AArch64 emit state: `state.emit`, formatted assembly emission, fresh-label
  generation, and register-cache invalidation.
- Operand and result helpers: `operand_to_x0`, `store_x0_to`, and `emit_int_cmp_insn`.
- Condition helpers: `arm_int_cond_code` and `arm_invert_cond_code`.
- Soft-float support for `F128` comparison.
- AArch64 register conventions for `x0`, `x1`, `x2`, `w0`, `w1`, `s0`, `s1`,
  `d0`, and `d1`.

## Hidden Assumptions

- `operand_to_x0` may clobber `x0`, so float comparison must save the left
  operand in `x1` before loading the right operand.
- Floating-point booleans are materialized through integer `x0` with `cset`.
- The archived float condition mapping is not the same as integer
  signed/unsigned condition-code selection; rebuild work must not blindly share
  the integer mapping for floats.
- `emit_int_cmp_insn` owns width and signedness details for integer compares.
- Fused integer branches rely on `arm_invert_cond_code` being correct for every
  condition returned by `arm_int_cond_code`.
- `emit_select_impl` assumes selecting through integer registers is acceptable
  for the values it is asked to lower.
- Register-cache invalidation is observable: full invalidation follows fused
  branches, while accumulator-only invalidation follows `select`.

## Known Failure Risks For Rebuild

- Rebuilding float comparisons with integer condition-code mapping can break
  ordered/unordered behavior and signedness-independent float comparisons.
- Forgetting to preserve the left floating-point operand before loading the
  right operand will compare the right operand against itself or stale data.
- Replacing the fused branch shape without checking fallthrough and label
  assumptions can invert branch behavior or leave stale register-cache state.
- Lowering `F128` locally instead of through the shared soft-float helper would
  split binary128 comparison ownership.
- Treating `select` as lazy can change side effects if operand materialization
  has backend-observable behavior.
- Omitting the register-cache invalidation after branches or `csel` can leave
  later emission using stale accumulator assumptions.

## Rebuild Guidance

Rebuild comparison lowering around explicit ownership boundaries:

1. Keep integer compare instruction selection and integer condition-code
   mapping in one shared AArch64 compare helper.
2. Keep float compare condition mapping separate from integer signedness
   mapping unless the semantics are proved equivalent.
3. Delegate `F128` comparison to the binary128 soft-float support rather than
   duplicating soft-float call policy here.
4. Preserve the observed register-cache invalidation points around fused
   branches and select lowering.
5. Add proof for both materialized boolean compares and fused branch/select
   users when replacing the archived behavior.
