Status: Active
Source Idea Path: ideas/open/854_lir_next_body_parameter_authority_handoff.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Select The Next Body-Parameter Authority Row

# Current Packet

## Just Finished

Lifecycle switched from exhausted 734 after accepted Step 7.42 to the separate
blocker idea 854. The preserved 734 return point is source completion
reassessment after commit `96853b2a2`; after 854 closes with an exact handoff,
reactivate 734 and repair it for one matching typed Raw-BIR receiver row.

## Suggested Next

Start Step 1 by tracing the next valid currently produced function-body
parameter use after the accepted DirectPointer pointer-truthiness row. Select
exactly one bounded semantic consumer relation that can be published and
verified natively.

## Watchouts

Do not edit Raw-BIR/importer code in 854. Do not reopen accepted
body-parameter rows or infer authority from text, names, signatures,
diagnostics, rendered operands, compatibility mirrors, or testcase shape.

## Proof

No 854 implementation proof yet. Accepted 734 Step 7.42 proof was the fresh
build plus focused `^backend_lir_to_bir_interface$` result in `test_after.log`,
`git diff --check`, and matching non-decreasing 1/1 regression guard.
