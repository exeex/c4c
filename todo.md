Status: Active
Source Idea Path: ideas/open/30_riscv_prepared_edge_publication_stack_destination_policy_broadening.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Validate the Stack-Destination Broadening Slice

# Current Packet

## Just Finished

Step 3 route review is resolved. The reviewer report at
`review/riscv_stack_destination_fail_closed_route_review.md` accepted the
Step 2 fail-closed packet as aligned with idea 30 and not testcase-overfit.
Lifecycle decision: proceed to validation and closure as a fail-closed policy
slice, while keeping actual scratch-register policy as durable follow-up scope.
The source idea now permits closure with the concrete blocker, and
`ideas/open/32_riscv_prepared_edge_publication_stack_destination_scratch_policy.md`
tracks the future source-to-stack broadening work.

## Suggested Next

Execute Step 4 validation. Refresh or restore the focused proof so root
`test_after.log` exists and matches the claim in this scratchpad, then run the
supervisor-selected broader backend proof before closing idea 30.

## Watchouts

Do not add scratch-register semantics in the validation packet. The helper
still supports only `Register -> StackSlot` for concrete 4-byte stack
destinations. `RematerializableImmediate -> StackSlot`,
`StackSlot -> StackSlot`, and `PointerBasePlusOffset -> StackSlot` remain
fail-closed until the follow-up idea defines an owned RISC-V scratch policy.

## Proof

Route review accepted Step 2 fail-closed policy, with one validation caveat:
the reviewer could not find root `test_after.log`. Step 4 must refresh
canonical validation evidence before closure.
