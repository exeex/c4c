Status: Active
Source Idea Path: ideas/open/303_aarch64_sign_extension_assembler_legality.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect Sign-Extension Route

# Current Packet

## Just Finished

Step 1 diagnostic completed for focused `00205`. The accepted baseline fails
assembly at `build/c_testsuite_aarch64_backend/src/00205.c.s:29` with:
`sxtw w9, w13`.

The emitting surface is `src/backend/mir/aarch64/codegen/dispatch.cpp`,
`lower_fused_compare_branch_from_emitted_cast(...)`. That helper recognizes a
fusable branch compare fed by a same-block `SExt` cast, then synthesizes an
inline assembler machine-node payload through
`make_branch_compare_assembler_instruction(...)`. It currently forces the cast
source, other compare operand, and extension result scratch/register through
`abi::RegisterView::W`; for an `SExt i32 -> i64` this emits the illegal
`sxtw` W-destination form and then compares W operands.

## Suggested Next

Delegate Step 2: repair
`lower_fused_compare_branch_from_emitted_cast(...)` so sign-extension spelling
and the following compare use register views derived from semantic widths.

## Watchouts

- Do not revert or overwrite the dirty Step 3 implementation files from
  idea 302.
- Do not claim focused pass-count progress from the current dirty code.
- Keep this owner scoped to AArch64 sign-extension width/spelling legality.
- Do not change expectations, allowlists, unsupported classifications,
  timeout policy, runner behavior, CTest registration, or proof-log policy.
- Semantic width rule for Step 2: `sxtw` is only legal as `sxtw Xd, Wn`.
  The extension destination/scratch must use the widened result view
  (`X` for 64-bit results, `W` only for <=32-bit `sxtb`/`sxth` results), while
  the source operand remains the narrow W view. The compare emitted after the
  extension must use the widened compare/result view for both operands; do not
  repair this as printer-only string substitution.

## Proof

No new proof log was produced for this diagnostic-only packet, per delegation.
Used existing `test_before.log` as the accepted baseline and inspected
`build/c_testsuite_aarch64_backend/src/00205.c.s` plus AST-backed lookup of the
dispatch helper. No `test_after.log` was created.
