# AArch64 I128 Operations Legacy Surface

This artifact preserves the useful production shape from the removed
`i128_ops.cpp` translation surface. The old file was not active C++; it was a
line-commented mirror of the reference Rust implementation at
`ref/claudes-c-compiler/src/backend/arm/codegen/i128_ops.rs`.

## Role

The surface implemented 128-bit integer lowering on `ArmCodegen` using paired
64-bit AArch64 registers.

It covered pair loads and stores, accumulator save/restore helpers, unary
operations, add/sub/mul/bitwise operations, variable and constant shifts,
runtime helper calls for division/remainder and float conversions, and compare
result materialization.

The old route treated `x0`/`x1` as the primary 128-bit accumulator pair and
used `x2`/`x3` plus `x4`/`x5` as prepared binary-operation operand pairs.

## Entry Points

- `emit_load_acc_pair_impl(op)`: materializes an operand into `x0` and `x1`.
- `emit_store_acc_pair_impl(dest)`: stores the accumulator pair to a
  destination value.
- `emit_store_pair_to_slot_impl(slot)`: stores `x0` and `x1` to adjacent
  stack-slot words.
- `emit_load_pair_from_slot_impl(slot)`: loads adjacent stack-slot words into
  `x0` and `x1`.
- `emit_save_acc_pair_impl()`: snapshots the accumulator pair into `x2` and
  `x3`.
- `emit_store_pair_indirect_impl()`: stores the saved pair from `x2` and `x3`
  through pointer `x9`.
- `emit_load_pair_indirect_impl()`: loads a pair through pointer `x9` into
  `x0` and `x1`.
- `emit_i128_neg_impl()` and `emit_i128_not_impl()`: emit unary pair
  operations.
- `emit_sign_extend_acc_high_impl()` and `emit_zero_acc_high_impl()`: normalize
  the high half after narrow integer materialization.
- `emit_i128_prep_binop_impl(lhs, rhs)`: prepares lhs and rhs pairs through the
  shared `prep_i128_binop` helper.
- `emit_i128_add_impl()`, `emit_i128_sub_impl()`, `emit_i128_mul_impl()`,
  `emit_i128_and_impl()`, `emit_i128_or_impl()`, and `emit_i128_xor_impl()`:
  emit the core inline binary operations.
- `emit_i128_shl_impl()`, `emit_i128_lshr_impl()`, and
  `emit_i128_ashr_impl()`: emit variable shift lowering with masked shift
  amounts.
- `emit_i128_prep_shift_lhs_impl(lhs)`: materializes the shift lhs into
  `x2`/`x3`.
- `emit_i128_shl_const_impl(amount)`, `emit_i128_lshr_const_impl(amount)`, and
  `emit_i128_ashr_const_impl(amount)`: emit constant shift lowering.
- `emit_i128_divrem_call_impl(func_name, lhs, rhs)`: marshals two i128
  operands and calls the selected runtime helper.
- `emit_float_to_i128_call_impl(src, to_signed, from_ty)`: calls the selected
  F32/F64 to i128 conversion helper.
- `emit_i128_to_float_call_impl(src, from_signed, to_ty)`: calls the selected
  i128 to F32/F64 conversion helper.
- `emit_i128_cmp_eq_impl(is_ne)` and `emit_i128_cmp_ordered_impl(op)`: produce
  scalar boolean comparison results.
- `emit_i128_store_result_impl(dest)` and
  `emit_i128_cmp_store_result_impl(dest)`: store pair or scalar comparison
  results back to IR destinations.

## Register Conventions

- `x0`: low 64 bits of the accumulator pair, scalar boolean result carrier,
  and first call argument for runtime helpers.
- `x1`: high 64 bits of the accumulator pair and second call argument for
  runtime helpers.
- `x2`: prepared lhs low word, saved accumulator low word, or runtime helper
  rhs low argument after reshuffling.
- `x3`: prepared lhs high word, saved accumulator high word, or runtime helper
  rhs high argument after reshuffling.
- `x4`: prepared rhs low word or variable shift amount.
- `x5`: prepared rhs high word or scratch for `64 - amount`.
- `x6`: scratch used by variable shift cross-lane assembly.
- `x9`: pointer carrier for indirect pair loads and stores.
- `s0`/`d0`: floating-point call argument or return carrier for F32/F64
  conversion helpers.

## Pair Transport

Pair storage used little-endian low/high word layout in adjacent 8-byte slots:
the low word lived at the base slot offset and the high word lived at
`offset + 8`.

Stack transport emitted ordinary `str`/`ldr` operations against `sp`.
Indirect transport assumed `x9` already held the target address and stored or
loaded two 64-bit words at `[x9]` and `[x9, #8]`.

The helper surface delegated value-level materialization and storage to
surrounding backend routines:

- `operand_to_x0_x1`
- `store_x0_x1_to`
- `emit_store_to_sp`
- `emit_load_from_sp`
- `prep_i128_binop`
- `store_x0_to`

## Core Arithmetic And Bitwise Lowering

Unary negation used two's-complement pair arithmetic:

```asm
mvn x0, x0
mvn x1, x1
adds x0, x0, #1
adc x1, x1, xzr
```

Bitwise NOT inverted both halves with `mvn`.

Addition and subtraction consumed prepared operand pairs in `x2`/`x3` and
`x4`/`x5`:

- add: `adds x0, x2, x4` followed by `adc x1, x3, x5`
- subtract: `subs x0, x2, x4` followed by `sbc x1, x3, x5`

Multiplication computed the low 128 bits of the product from partial products:

- `mul x0, x2, x4` for the low 64 bits
- `umulh x1, x2, x4` for the high half of the low-limb product
- `madd x1, x3, x4, x1` for lhs-high times rhs-low
- `madd x1, x2, x5, x1` for lhs-low times rhs-high

Bitwise AND, OR, and XOR were lane-wise over the low and high halves.

## Shift Lowering

Variable shifts masked the shift amount with `#127`, handled zero as a no-op,
split on `amount >= 64`, and used labels from `state.fresh_label`.

For amounts from 1 through 63:

- left shift moved the high word left, ORed in the low word shifted right by
  `64 - amount`, and shifted the low word left.
- logical right shift moved the low word right, ORed in the high word shifted
  left by `64 - amount`, and shifted the high word right.
- arithmetic right shift used the same low-word cross-lane assembly as logical
  right shift, but shifted the high word with `asr`.

For amounts from 64 through 127:

- left shift put `x2 << (amount - 64)` in `x1` and zeroed `x0`.
- logical right shift put `x3 >> (amount - 64)` in `x0` and zeroed `x1`.
- arithmetic right shift put `x3 asr (amount - 64)` in `x0` and sign-filled
  `x1` from bit 63 of `x3`.

Constant shifts used the same semantic cases after masking the immediate with
`127`, but emitted direct immediate forms and explicit special cases for `0`,
`64`, and `> 64`.

## Runtime Helper Boundaries

Division and remainder were not emitted inline. The old surface loaded the lhs
into `x0`/`x1`, saved it to `x2`/`x3`, loaded the rhs into `x0`/`x1`, copied it
to `x4`/`x5`, restored lhs to `x0`/`x1`, moved rhs to `x2`/`x3`, and branched
to the caller-provided helper symbol.

Float conversion helpers were selected from fixed F32/F64 and signed/unsigned
matrices:

- float to signed i128: `__fixdfti` or `__fixsfti`
- float to unsigned i128: `__fixunsdfti` or `__fixunssfti`
- signed i128 to float: `__floattidf` or `__floattisf`
- unsigned i128 to float: `__floatuntidf` or `__floatuntisf`

F32 operands crossed between integer and FP registers with `fmov s0, w0` or
`fmov w0, s0`. F64 operands crossed with `fmov d0, x0` or `fmov x0, d0`.
Conversion helper calls invalidated the register cache.

## Comparison Lowering

Equality and inequality XORed each half, ORed the mismatch bits together,
compared the result with zero, and used `cset eq` or `cset ne`.

Ordered comparisons compared high words first, set a provisional result from
the high-word condition, and skipped the low-word comparison when the high words
differed.

The old mapping was:

- signed less/less-equal: high condition `lt`, low condition `lo` or `ls`
- signed greater/greater-equal: high condition `gt`, low condition `hi` or
  `hs`
- unsigned less/less-equal: high condition `lo`, low condition `lo` or `ls`
- unsigned greater/greater-equal: high condition `hi`, low condition `hi` or
  `hs`

Equality operations were not expected in `emit_i128_cmp_ordered_impl`.

## Dependencies

The removed surface depended on these surrounding concepts:

- IR model: `IrCmpOp`, `Operand`, `Value`, `IrType`, and `StackSlot`.
- Backend pair materialization and storage helpers such as `operand_to_x0_x1`,
  `store_x0_x1_to`, and `prep_i128_binop`.
- Scalar result storage through `store_x0_to`.
- Stack access helpers using `sp`-relative offsets.
- Backend assembly emission through `state.emit` and `state.emit_fmt`.
- Label generation through `state.fresh_label`.
- Register-cache invalidation after runtime helper calls.
- Runtime/compiler-rt helper availability for i128 div/rem and F32/F64
  conversions.

## Hidden Assumptions

- 128-bit integers are always represented as low/high 64-bit pairs.
- The low half is always in the lower-addressed stack slot and the lower
  numbered call argument register.
- `prep_i128_binop` must leave lhs in `x2`/`x3` and rhs in `x4`/`x5`.
- Variable shift amounts are carried in `x4` and may be destructively masked.
- Constant and variable shifts intentionally mask amounts with `127`.
- The high word of a signed narrow value can be sign-filled with
  `asr x1, x0, #63`; unsigned values can zero it.
- Runtime helper calls follow an ABI compatible with arguments
  `(x0, x1, x2, x3)` and return i128 in `x0`/`x1` or scalar FP in `s0`/`d0`.
- F32 and F64 values are carried as raw integer bits until explicitly moved to
  FP registers.
- The ordered-compare high-word condition is sufficient to decide the result
  whenever high halves differ.

## Known Failure Risks For Rebuild

- Swapping low and high halves would corrupt every operation, but would be
  especially hard to detect in pair transport and helper-call marshaling.
- Rebuilding multiplication as a full 256-bit product rather than truncating to
  low 128 bits would drift from the old contract.
- Forgetting carry or borrow propagation in add/sub would only fail on
  cross-limb boundary cases.
- Using logical high-word comparisons for signed ordered compares would break
  negative i128 ordering.
- Treating arithmetic right shift like logical right shift would fail
  sign-extension cases for negative values.
- Missing the `amount & 127` normalization would make out-of-range shifts
  diverge from the old route.
- Calling conversion helpers without `fmov` bit bridges would pass integer
  register payloads as the wrong floating-point values.
- Omitting register-cache invalidation after helper calls would preserve stale
  backend register facts across calls that clobber registers.
- Reusing `x9` before indirect load/store pair helpers would send pair
  transport through the wrong address.

## Rebuild Guidance

Rebuild this surface around an explicit pair-lowering contract:

`i128` lowering should carry low/high pair identity through structured target
MIR and machine instruction nodes for arithmetic, shifts, helper calls, and
transport. Final instruction spelling and helper-call text are printer or
encoding/object concerns only.
Pair homes, helper-call resources, and spill/reload materialization must be
represented by the shared allocation result in `../ALLOCATION_CONTRACT.md` so
the low/high lanes do not create a separate allocation policy.

1. Keep pair layout and allocation-result ownership centralized so arithmetic,
   shifts, helper calls, and storage agree on low/high ordering.
2. Separate semantic i128 lowering from generic stack and indirect pair
   transport when possible.
3. Treat helper-call marshaling as ABI-sensitive code and test both argument
   order and result placement.
4. Cover boundary cases for carry, borrow, signed ordering, shift amounts
   `0`, `63`, `64`, `65`, `127`, and masked amounts above `127`.
5. Keep raw-bit FP register bridging visible wherever i128/float conversion
   helpers remain in use.
