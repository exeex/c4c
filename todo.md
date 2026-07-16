Status: Active
Source Idea Path: ideas/open/860_lir_next_body_parameter_authority_handoff.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Select and publish one next body-parameter authority row

# Current Packet

## Just Finished

Completed 860 Step 1 for exactly one producer-side body-parameter authority
row: DirectScalar floating binary `fsub` RHS (`double f(double x) { return
2.0 - x; }`). LIR lowering now publishes native structured
`LirScalarBinaryRhsParameterAuthority` for the selected RHS parameter when no
LHS authority owns the row, the verifier accepts only the selected `fsub` RHS
form alongside existing RHS rows, and focused positive/malformed coverage was
added.

## Suggested Next

Continue 860 Step 1 by selecting exactly one next valid producer-side
body-parameter authority row after DirectScalar floating binary RHS `fadd`,
`fmul`, and `fsub`, then implement only that LIR producer/verifier/test handoff.

## Watchouts

Do not edit Raw-BIR receiver code, reopen accepted DirectPointer or
DirectScalar receipts, select multiple rows, recover authority from
presentation text, or absorb memory/VA, aggregate/vector,
module/type/global/metadata, residual instruction/terminator, inline-assembly,
ABI-expanded or aggregate parameters, or target-lowering work.

The RHS malformed opcode probes now use unsupported `fdiv`; `fsub` is a valid
selected RHS authority row and should not be used as the invalid-opcode probe
for existing RHS rows.

## Proof

Passed:

`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_lir_function_signature_type_ref$'; } > test_after.log 2>&1`

Supervisor also ran full suite:

`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure; } > test_after.log 2>&1`

Result: `3038/3038` passed. Focused before/after guard was not applicable
because the focused subset was green before and after, so the checker reported
no strict pass-count increase rather than a new failure.
