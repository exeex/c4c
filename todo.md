Status: Active
Source Idea Path: ideas/open/864_lir_next_non_body_parameter_authority_handoff.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Select and hand off one next non-body-parameter authority row

# Current Packet

## Just Finished

No accepted packet yet for `plan.md` Step 1. A rejected executor attempt
selected
`LirCallOp.structured_args[0].fixed_direct_call_argument_parameter_authority`,
but fixed direct-call argument 0 was already accepted by 734 Step 7.40, so 864
remains active at Step 1.

## Suggested Next

Continue `plan.md` Step 1 by selecting exactly one valid current-LIR
non-body-parameter authority row not already accepted by 734 through Step 7.50.
Document why the selected row is unaccepted, producer-ready, and bounded to one
handoff.

## Watchouts

Do not select fixed direct-call argument 0 or fixed direct-call argument 1
parameter authority. Idea 734 already accepted argument 0 in Step 7.40 and
argument 1 in Step 7.41, and backend receiver coverage already includes
`test_fixed_direct_call_argument0_parameter_authority_receipt_and_rejections`.
Do not touch Raw-BIR receiver/importer/container/verifier code while 864 is
active.

## Proof

Required for the next accepted Step 1 packet: fresh build, focused
producer/verifier proof for the newly selected row, and `git diff --check`.
