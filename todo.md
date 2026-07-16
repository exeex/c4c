Status: Active
Source Idea Path: ideas/open/865_lir_next_non_body_parameter_authority_handoff.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Trace and select one next non-body-parameter authority row

# Current Packet

## Just Finished

Plan-owner rejected closure for 734 after accepted Step 7.51 because the source
completion gate remains unmet. Idea 734 is parked after receiver commit
`750b6b3ba`, and this active runbook now owns the separate producer-side
blocker for exactly one next non-body-parameter authority handoff.

## Suggested Next

Trace current LIR producer/verifier behavior, choose exactly one valid
non-body-parameter row not already accepted by 734 through Step 7.51, and
record its native authority tuple plus malformed matrix before implementation.

## Watchouts

Do not edit Raw-BIR/importer receiver code, reopen accepted Step 7.50 or 7.51
call-result receipts, reopen fixed direct-call argument 0/1 parameter
receipts, choose a function-body parameter row, recover authority from
presentation text, or absorb memory/VA, aggregate/vector,
module/type/global/metadata, CFG/PHI, residual instruction/terminator,
inline-assembly, final convergence, generic residual sweeps, or any other
family.

## Proof

Pending for 865. Required proof after implementation: fresh build, focused
producer/verifier proof for the selected row, `git diff --check`, and any
matching regression guard required by touched shared LIR verifier or producer
code.
