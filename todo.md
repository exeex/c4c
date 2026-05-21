Status: Active
Source Idea Path: ideas/open/381_aarch64_shift_promotion_codegen_scalability_timeout.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Reconfirm Stage and First Slow Phase

# Current Packet

## Just Finished

Lifecycle switch complete. Umbrella idea 295 classified both timeout residuals
and was deactivated before implementation work:

- `00200` split to active idea 381 as backend-native AArch64 asm-generation
  timeout for expanded shift/type-promotion CFG/codegen scalability.
- `00207` split to parked idea 382 as generated-program runtime timeout from
  dynamic stack/VLA fixed-slot addressing across a moving `sp`.

## Suggested Next

Execute `plan.md` Step 1 for idea 381: rerun the delegated bounded `00200`
proof, confirm the timeout is still backend-native asm generation, and
localize the first slow backend phase before making implementation changes.

## Watchouts

Do not implement under umbrella idea 295. Do not work on parked idea 382 unless
the supervisor switches lifecycle state. Reject testcase-specific shortcuts,
timeout policy changes, runner changes, CTest registration changes,
unsupported-list changes, expectation weakening, proof-log policy changes, and
count-only progress claims.

## Proof

Lifecycle-only routing change. No build or test proof was run by the plan
owner.
