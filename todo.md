# Current Packet

Status: Active
Source Idea Path: ideas/open/808_lir_phi_scalar_bit_not_xor_authority.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Reconfirm the bounded scalar bit-not handoff and coverage

## Just Finished

- Lifecycle switch: 807 closed capability-complete after accepted focused
  proof in `8f31e2535`; 808 is now active as the next separately scoped 806
  dependency.

## Suggested Next

- Execute Step 1: reproduce and trace the scalar `UnaryOp::BitNot` / `xor`
  producer handoff, then select nearby positive and malformed-authority
  coverage without changing code.

## Watchouts

- Keep this route limited to scalar bit-not `xor`. Do not reopen floating or
  scalar unary-minus, postfix old-value, PHI/ternary consumer, or verifier
  contracts.

## Proof

- No 808 proof has been run. Step 2 must use a fresh build plus focused
  same-family coverage; after acceptance, 806 still requires the 100% full
  baseline before 804 can resume.
