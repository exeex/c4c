# Current Packet

Status: Active
Source Idea Path: ideas/open/807_lir_phi_floating_unary_minus_fneg_authority.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Reconfirm the bounded floating-minus handoff and choose coverage

## Just Finished

- Active plan switched from 806 after accepted Steps 1–2; 806 is parked at
  Step 3 pending the separately scoped 807 and 808 successor routes.

## Suggested Next

- Trace the floating `UnaryOp::Minus` / `fneg` result in `ieee/pr50310.c` and
  identify the paired positive and malformed-authority focused coverage.

## Watchouts

- Keep this packet limited to floating `fneg`. Do not absorb postfix,
  scalar-integer-minus, scalar bit-not, PHI-side, or text-recovery work.

## Proof

- Before code changes, reproduce the targeted failure narrowly. After the
  repair packet, require a fresh build plus focused same-family positive and
  malformed-authority proof; 807 does not itself clear the 806/804 full
  baseline gate.
