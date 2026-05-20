Status: Active
Source Idea Path: ideas/open/316_aarch64_frame_slot_layout_consistency.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize The Frame Layout Divergence

# Current Packet

## Just Finished

Activated idea 316, "AArch64 Frame Slot Layout Consistency", as the current
runbook.

## Suggested Next

Execute Step 1 by localizing the first frame-layout divergence for
`c_testsuite_aarch64_backend_src_00216_c` / `test_correct_filling`. Start from
generated artifacts and prepared frame/stack dumps, then record the exact
frame size, slot offsets, emitted prologue, accessed addresses, and owner
classification here before implementation.

## Watchouts

Do not reopen idea 314's legal large-offset instruction spelling unless new
evidence shows spelling is again the first bad fact. Keep the unrelated
transient `review/326_stdarg_byval_route_review.md` untouched.

Do not use filename-specific, function-specific, literal-offset-specific, or
expected-output-only fixes. This plan owns the general AArch64 frame
size/frame-slot offset consistency contract.

## Proof

Lifecycle activation only. No build or CTest proof was run for activation.
