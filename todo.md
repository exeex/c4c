Status: Active
Source Idea Path: ideas/open/854_lir_next_body_parameter_authority_handoff.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Publish And Verify The Selected Authority

# Current Packet

## Just Finished

Step 2 confirmed the selected unary floating `fneg` row already had the native
LIR authority carrier, emitter, and verifier contract: original parameter
`LirValueId`, current-function owner, parameter index, matching `LirTypeRef`,
`DirectScalar` ABI, explicit `Lhs` role, and unary `LirBinOp` consumer
coherence with opcode `fneg`, parameter `lhs`, and empty `rhs`.

The packet added focused malformed-authority coverage for that selected row in
`tests/frontend/frontend_lir_function_signature_type_ref_test.cpp`: omitted
authority, missing definition, invalid value, duplicate definition, foreign
owner, wrong index, wrong type, wrong ABI, wrong role, non-`fneg` consumer,
LHS mismatch, and populated `rhs` all fail closed before downstream use.

## Suggested Next

Supervisor should choose the next bounded 854 packet. This packet did not
select or start an adjacent parameter-use row.

## Watchouts

No code churn was needed in `src/codegen/lir/ir.hpp`,
`src/codegen/lir/verify.cpp`, or
`src/codegen/lir/hir_to_lir/expr/misc.cpp`; the existing authority path already
published and verified the selected row. Raw-BIR receipt and adjacent parameter
rows remain out of scope.

## Proof

Ran delegated proof:
`( cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_lir_function_signature_type_ref$' ) > test_after.log 2>&1 && git diff --check`.

Result: passed. Proof log: `test_after.log`.
