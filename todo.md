Status: Active
Source Idea Path: ideas/open/856_lir_next_body_parameter_authority_handoff.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Trace And Select One Next Body-Parameter Use Authority Row

# Current Packet

## Just Finished

Switched lifecycle from 734 after accepted Step 7.44 receiver commit
`e0540da75`. 734 is parked with a durable post-Step 7.44 close-rejected
resumption record and returns only after this idea publishes one exact next
body-parameter authority handoff.

## Suggested Next

Execute `plan.md` Step 1: trace the remaining current LIR body-parameter
matrix after 734 Step 7.44 and select exactly one next valid row, or identify
the first missing native authority gap. Do not start Raw-BIR receiver work.

## Watchouts

Do not reopen accepted DirectPointer or DirectScalar body-parameter receipts
through 734 Step 7.44. Do not derive authority from text, names, rendered
operands, signatures, diagnostics, compatibility mirrors, `monostate`, or
testcase shape.

## Proof

Lifecycle-only switch. Required check: `git diff --check`.
