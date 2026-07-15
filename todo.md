# Current Packet

Status: Active
Source Idea Path: ideas/open/754_lir_aggregate_vector_value_identity_convergence.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prove and hand off the bounded row

## Just Finished

754 Step 3 accepted in `a351cde2a`: the selected native direct-complex
`LirExtractValueOp` carries a structured result-element type and verifies it
against 801's ordered anonymous aggregate fields. Negative, out-of-range,
missing-type, and type-conflict extracts reject; direct-complex positive
coverage passes. Fresh targeted proof passed 3/3 and the non-regression guard
passed.

## Suggested Next

Supervisor-owned Step 4: run the selected broader checkpoint and obtain a
100% passing full baseline before any semantic closure decision. Record the
bounded-row handoff evidence only if accepted; do not expand to other
aggregate/vector rows, receiver work, or Raw BIR.

## Watchouts

The result-element fact is populated only by the selected native unary
real/imag direct-complex lowering path. Do not recover layout or element types
from display text, widen it to other extract rows, reopen 801, weaken
result/use/layout checks, or perform Raw-BIR, CFG/PHI, lowering, MIR, or
emission work.

## Proof

`cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^(frontend_hir_tests|frontend_lir_call_type_ref|frontend_lir_function_signature_type_ref)$'`
passed 3/3; proof log: `test_after.log`.
