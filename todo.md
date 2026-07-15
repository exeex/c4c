# Current Packet

Status: Active
Source Idea Path: ideas/open/754_lir_aggregate_vector_value_identity_convergence.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Verify selected LirExtractValueOp index and result-type coherence

## Just Finished

Step 3 selected-row coverage completed: the existing verifier uses 801's
ordered anonymous-aggregate layout facts to reject negative/out-of-range
`LirExtractValueOp` indexes and mismatched result-element types. Nearby
direct-complex positive and malformed checks now exercise that same selected
row without display-text recovery or widening.

## Suggested Next

Step 4 only: obtain the supervisor-owned full baseline and record the bounded
selected-row handoff; do not widen into any other aggregate/vector route.

## Watchouts

Do not revisit Steps 2–3 authority, widen to other aggregate/vector rows,
reopen PHI work, parse display text, or absorb layout publication work owned
by 801.

## Proof

Step 3 exact focused proof passed 1/1:
`cmake --build --preset default --target frontend_lir_call_type_ref_test && ctest --test-dir build --output-on-failure -R '^frontend_lir_call_type_ref$' > test_after.log 2>&1`.
The proof log is `test_after.log`.
