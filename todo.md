Status: Active
Source Idea Path: ideas/open/332_aarch64_movi_zero_extension_materialization.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize The MOVI Sign-Extension Owner

# Current Packet

## Just Finished

Completed plan Step 1, "Localize The MOVI Sign-Extension Owner". The first
generated-code divergence is semantic BIR lowering of folded integer casts, not
AArch64 instruction selection, ABI call placement, or `pll` output.

Evidence:

- Source `tests/c/external/c-testsuite/src/00204.c:383` declares
  `pll(unsigned long long x)` and `movi()` passes unsuffixed hex constants to it
  at lines 391-430.
- HIR preserves the first bad value correctly: `movi()` prints
  `pll(2882338816)` for source `0xabcd0000`, `pll(2863311530)` for
  `0xaaaaaaaa`, and `pll(4177066232)` for `0xf8f8f8f8`.
- LIR/LLVM for `movi()` also preserves the required unsigned conversion:
  `%t4 = zext i32 2882338816 to i64`, `%t7 = zext i32 2863311530 to i64`,
  and `%t9 = zext i32 4177066232 to i64`, each immediately passed to
  `pll(i64 ...)`.
- Semantic BIR is the first bad artifact. The same calls become
  `bir.call void pll(i64 -1412628480)`,
  `bir.call void pll(i64 -1431655766)`, and
  `bir.call void pll(i64 -117901064)`, which are the sign-extended 64-bit
  forms `ffffffffabcd0000`, `ffffffffaaaaaaaa`, and `fffffffff8f8f8f8`.
- Prepared BIR carries those bad immediates unchanged into the same-module
  `pll` call records: for `movi` callsite block 0 inst 3, `arg0 bank=gpr
  from=immediate:-1412628480 to=x0`; inst 12 similarly passes
  `from=immediate:-1431655766 to=x0`; inst 16 passes
  `from=immediate:-117901064 to=x0`.
- Generated AArch64 assembly is a faithful materialization of the already-bad
  BIR immediates: the first bad call emits `movz x0, #43981, lsl #16` followed
  by `movk x0, #65535, lsl #32` and `movk x0, #65535, lsl #48`, then
  `bl pll`, producing `0xffffffffabcd0000` in ABI register `x0`.

Owner classification: `src/backend/bir/lir_to_bir/scalar.cpp` immediate cast
folding. `BirFunctionLowerer::lower_value(..., I32, ...)` parses the decimal
LIR operand and returns `bir::Value::immediate_i32(static_cast<int32_t>(...))`,
so values above `INT32_MAX` are represented as negative 32-bit payloads. The
later `LirCastKind::ZExt` fast path at `lower_scalar_compare_inst` lines
506-524 handles both `SExt` and `ZExt` by copying `value->immediate` into
`bir::Value::immediate_i64(imm)`, bypassing the existing `fold_integer_cast`
logic that would mask and zero-extend for `ZExt`. Source width and signedness
are correct through HIR/LIR; the bad high bits are introduced when BIR folds
`zext i32 immediate to i64`.

## Suggested Next

Execute plan Step 2 by repairing BIR immediate cast folding generally. The
smallest credible repair is to route folded `LirCastKind::ZExt`/`SExt`/`Trunc`
immediates through `fold_integer_cast` or equivalent width-aware masking in
`src/backend/bir/lir_to_bir/scalar.cpp`, then add focused BIR/backend coverage
for `zext i32 2882338816 to i64` and at least one nearby high-bit 32-bit
literal such as `0xaaaaaaaa`.

## Watchouts

Do not repair this in the AArch64 machine printer or immediate materializer:
assembly is only reflecting the already-bad BIR value. Do not special-case
`00204.c`, `movi`, `abcd0000`, `aaaaaaaa`, `x0`, or a `movz`/`movk` sequence.
Keep HFA overflow assignment, scalar-FP symbol-load placement, byval aggregate
lane publication, stdarg cursor, fixed-formal entry publication, and
local/value-home publication out of scope unless new generated evidence moves
the first bad fact before the BIR `zext` fold. Leave
`review/326_stdarg_byval_route_review.md` untouched.

## Proof

Todo-only localization; no full build was needed. Evidence commands used:

- `build/c4cll --dump-hir tests/c/external/c-testsuite/src/00204.c`
- `build/c4cll --target aarch64-linux-gnu tests/c/external/c-testsuite/src/00204.c -o /tmp/00204.ll`
- `build/c4cll --target aarch64-linux-gnu --dump-bir tests/c/external/c-testsuite/src/00204.c`
- `build/c4cll --target aarch64-linux-gnu --dump-prepared-bir tests/c/external/c-testsuite/src/00204.c`
- read-only inspection of `build/c_testsuite_aarch64_backend/src/00204.c.s`
  and current `test_after.log`

Suggested Step 2 proof command after a code repair:
`cmake --build build --target c4cll backend_prepared_printer_test backend_aarch64_scalar_cast_records_test && ctest --test-dir build -j --output-on-failure -R 'backend_(cli_dump_(bir|prepared_bir)_00204_stdarg|prepared_printer|aarch64_scalar_cast_records|aarch64_backend_src_00204_c)'`.
