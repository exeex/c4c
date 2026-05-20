Status: Active
Source Idea Path: ideas/open/335_aarch64_uninitialized_local_slot_runtime_residual.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Uninitialized Local-Slot First Bad Fact

# Current Packet

## Just Finished

Lifecycle review closed idea 334 and activated the split residual idea 335. No
executor packet has run for this plan yet.

## Suggested Next

Execute Step 1: localize the first uninitialized local-slot fact for
`c_testsuite_aarch64_backend_src_00164_c`, mapping the runtime mismatch back to
generated loads, stack slots, source values, and expected stores.

## Watchouts

Do not reopen scalar operand-fact printing unless the old printer diagnostics
return. Do not special-case `00164.c`, stack offsets around `sp+#148`,
`sp+#152`, or `sp+#156`, one instruction index, or one printed value. Keep
parked frame-layout idea 316 out of scope unless current localization proves
frame allocation or slot layout is the first owner.

## Proof

Lifecycle-only activation. Close gate for idea 334 used the existing focused
regression guard logs: `test_before.log` had 6/2/8 and `test_after.log` had
7/1/8 with no new failures.
