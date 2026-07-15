# Current Packet

Status: Active
Source Idea Path: ideas/open/754_lir_aggregate_vector_value_identity_convergence.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Verify row-specific index or mask facts

## Just Finished

754 Step 3 complete: the selected native direct-complex `LirExtractValueOp`
now carries a structured result-element type and verifies it against 801's
ordered anonymous aggregate fields. Negative, out-of-range, missing-type, and
type-conflict extracts reject; direct-complex positive coverage passes.

## Suggested Next

Step 4 only: obtain the bounded-row proof and handoff evidence; do not expand
to other aggregate/vector rows or receiver work.

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
