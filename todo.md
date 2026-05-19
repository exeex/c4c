Status: Active
Source Idea Path: ideas/open/303_aarch64_sign_extension_assembler_legality.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect Sign-Extension Route

# Current Packet

## Just Finished

Lifecycle switch only. Idea 302 is parked because the exposed `00205`
`sxtw w9, w13` failure is outside its scalar arithmetic/reduction
machine-node operand-form scope.

## Suggested Next

Delegate Step 1: inspect the sign-extension route that emits `sxtw` with a W
destination register, currently observed in `00205`.

## Watchouts

- Do not revert or overwrite the dirty Step 3 implementation files from
  idea 302.
- Do not claim focused pass-count progress from the current dirty code.
- Keep this owner scoped to AArch64 sign-extension width/spelling legality.
- Do not change expectations, allowlists, unsupported classifications,
  timeout policy, runner behavior, CTest registration, or proof-log policy.

## Proof

No validation run by plan owner; lifecycle routing only.
