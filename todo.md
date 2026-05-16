# Current Packet

Status: Active
Source Idea Path: ideas/open/255_aarch64_alu_legacy_semantic_lowering_followup.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Legacy ALU Routes Against Current Pipeline

## Just Finished

Step 1 - Audit Legacy ALU Routes Against Current Pipeline completed as a
survey-only classification packet.

Route classification:

- `emit_float_neg_impl`: separate-idea material. Current AArch64 MIR has
  `ScalarFpUnaryIntrinsicRecord` for prepared scalar FP unary intrinsics and
  printer support for selected `FAbs`; the deleted route's raw integer/FP
  `x0`/`s0`/`d0` bit shuttle is not current semantic authority. Reintroducing
  floating negation should be a scalar-FP-unary intrinsic/helper initiative,
  not part of this integer ALU packet series.
- `emit_int_neg_impl` and `emit_int_not_impl`: accepted for this plan, but only
  through typed scalar MIR. Current surfaces are `ScalarInstructionRecord`,
  `ScalarAluRecord`, allocation-backed `result_register`, `OperandRecord`
  inputs, `make_scalar_instruction`, and `make_scalar_alu_print_lines`; the
  legacy `x0` accumulator spelling is obsolete.
- `emit_int_clz_impl`: accepted for this plan. It should become an ALU-owned
  structured unary/bit operation with explicit 32-bit versus 64-bit register
  view selection, using prepared value homes/storage and printer spelling
  behind the scalar ALU surface.
- `emit_int_ctz_impl`: accepted for this plan. Preserve the `rbit` then `clz`
  sequence as printer/lowering behavior behind typed records; do not encode it
  as final-assembly string matching.
- `emit_int_bswap_impl`: accepted for this plan. Preserve the 16-bit `rev`
  plus right-shift behavior explicitly; `I32`/`I64` forms can map to `rev`
  through typed width facts and allocation-backed result/source registers.
- `emit_int_popcount_impl`: accepted for this plan only as structured lowering
  with explicit SIMD/FPR temporary requirements. The old `v0`/`s0` convention
  is obsolete unless a prepared scratch/temporary policy selects those
  registers.
- Unsigned power-of-two `UDiv`/`URem` reductions: accepted for this plan.
  Current BIR `BinaryInst` opcodes and immediate operands can identify
  `UDiv`/`URem` with constant RHS; implement only unsigned reductions through
  typed shift/mask records. Signed power-of-two division/remainder remains
  separate design material unless a later source idea proves signed semantics.
- Accumulator fallback division/remainder/variable-shift behavior: accepted as
  semantic coverage, obsolete as an `x0`/`x1`/`x2`/`x3` accumulator route.
  Current mapping must use allocation facts, explicit result/source operands,
  and prepared scratch resources for `SDiv`, `UDiv`, `SRem`, `URem`, `Shl`,
  `LShr`, and `AShr`.
- Register-direct scratch handling: accepted as allocation-conflict handling,
  obsolete as `dest_reg`/callee-saved text-emitter authority. Use
  `result_register`, register operand occupancy, and prepared scratch facts to
  handle right-side destination-register conflicts before overwriting a result
  register.
- 32-bit extension behavior: accepted as an explicit post-operation contract.
  Current scalar cast printing already models signed and zero extension for
  casts; ALU routes that introduce 32-bit arithmetic/div/rem/shift behavior
  must model signed `sxtw` and unsigned W-register zero-extension deliberately.
- `emit_copy_i128_impl`: accepted as semantic coverage only if it preserves
  both low and high lanes through current i128 surfaces. Current mapping is
  `I128TransportRecord`, `I128PairOperationRecord`, `I128ShiftRecord`,
  `I128RuntimeHelperBoundaryRecord`, prepared i128 carriers, and low/high lane
  printer helpers; scalar `x0`-only copy is obsolete.

## Suggested Next

First implementation packet recommendation: start with Step 2 by adding the
smallest ALU-owned structured unary/bit-operation slice for integer negation,
bitwise not, and CLZ over 32-bit and 64-bit register views. Owned code should
extend the scalar ALU record/selection/printer path rather than recreating
`alu.md` or using final assembly string shortcuts. The focused proof subset
should be:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_aarch64_(scalar_alu_records|prepared_scalar_alu_records|machine_printer|instruction_dispatch|scalar_record_contract)$'
```

## Watchouts

- Do not reopen ALU markdown redistribution or recreate `alu.md`.
- Do not revive text-emitter accumulator conventions as semantic authority.
- Do not use named-case shortcuts, final assembly string matching, expectation
  rewrites, or unsupported downgrades to claim progress.
- Keep signed power-of-two division/remainder separate from unsigned reduction
  unless signed semantics are separately designed and proved.
- Preserve explicit 32-bit extension, scratch-conflict, and i128 high-half
  requirements when those routes are classified as accepted.
- Current integer scalar ALU support is narrower than the operation enum:
  `Add`, `Sub`, `And`, `Or`, and `Xor` are selected as integer operations, but
  the current printer path only accepts printable add/sub integer nodes.
- `Mul`, `SDiv`, `UDiv`, `SRem`, `URem`, and variable shifts currently map to
  operation names or BIR opcodes but are not accepted integer scalar ALU
  lowering/printer coverage.
- Do not treat old register names (`x0`, `x1`, `x2`, `x3`, `s0`, `d0`, `v0`)
  as allocation authority; they are only valid if selected by prepared
  allocation or explicit scratch facts.

## Proof

No build required or run for this survey-only packet. No implementation files,
tests, expectations, `plan.md`, source ideas, or `review/` files were edited.
