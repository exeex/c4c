Status: Active
Source Idea Path: ideas/open/365_aarch64_signed_remainder_lowering.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Signed Remainder Operand Flow

# Current Packet

## Just Finished

Step 1 localized the signed remainder operand-flow failure for `00143`.
Source `tests/c/external/c-testsuite/src/00143.c` computes
`switch (count % 8)` after `count = 39`, so the selector must be `7`.
Prepared BIR still carries the correct operation:
`%t20 = bir.load_local i32 %lv.count` followed by
`%t21 = bir.srem i32 %t20, 8`; `%t21` is assigned to result register `x13/w13`.

The first bad generated sequence is in
`build/c_testsuite_aarch64_backend/src/00143.c.s` lines 778-782:
`ldr x13, [x9]` loads the dividend home for `%t20`/`count` into `x13`,
`mov w9, #8` materializes the divisor immediate, then
`sdiv w9, w13, w9` writes the quotient `39 / 8 == 4` into `w9`.
The following `msub w13, w9, w9, w13` uses `w9` as both quotient and divisor,
computing `39 - 4 * 4 == 23` instead of `39 - 4 * 8 == 7`.

AST-backed lookup localizes the owning backend boundary to
`src/backend/mir/aarch64/codegen/alu.cpp`. `make_prepared_scalar_alu_record`
at line 3345 preserves the correct `SRem` record with lhs `%t20`, immediate
rhs `8`, and result `%t21`/`w13`; `make_prepared_scalar_operand` at line 3173
correctly represents the RHS as an immediate. The original divisor is lost in
`make_scalar_alu_print_lines` at line 2163, specifically the integer
mul/div/rem printing path around lines 2296-2475. For a remainder with an
immediate RHS, the RHS materialization selects scratch `w9`. Because the result
register `w13` aliases the dividend register, the remainder path then asks
`scalar_gp_scratch_name` for a quotient scratch without marking the already
chosen immediate-divisor scratch as occupied, so it reselects `w9`. The `sdiv`
therefore overwrites the only copy of the divisor before `msub`.

## Suggested Next

Proceed to Step 2 with focused machine-printer coverage for signed remainder
where the result aliases the dividend and the divisor is an immediate. The
assertion should require distinct quotient and divisor carriers in the emitted
`sdiv`/`msub`, e.g. `sdiv <quot>, <dividend>, <divisor>` followed by
`msub <result>, <quot>, <same original divisor>, <dividend>`.

## Watchouts

Do not reopen synthetic label uniqueness, switch lowering, or
Duff's-device-specific behavior; the selector failure is caused before the
switch compares. The repair should be general for scalar remainder printing,
and may need to consider unsigned `URem` too because the same print path shares
the immediate-RHS divisor and quotient scratch allocation logic.

## Proof

Ran the delegated proof:

```sh
ctest --test-dir build -j --output-on-failure -R '^c_testsuite_aarch64_backend_src_00143_c$' > test_after.log 2>&1
```

Result: failed as expected for localization. `test_after.log` reports
`[RUNTIME_NONZERO]` for `tests/c/external/c-testsuite/src/00143.c`, exit `1`.
