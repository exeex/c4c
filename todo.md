Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 7.49
Current Step Title: Receive the one 860-authorized DirectScalar binary-fsub-RHS parameter authority row

# Current Packet

## Just Finished

Completed `plan.md` Step 7.49: received the closed-860
`LirBinOp.scalar_rhs_parameter_authority` DirectScalar floating binary-`fsub`
RHS row into typed Raw BIR, preserving the original parameter tuple, RHS role,
RHS operand identity, operation/type, and nonselected scalar LHS coherence.

## Suggested Next

Select the next unopened 734 receiver row after Step 7.49, without repeating
the accepted DirectScalar binary-`fsub` RHS parameter-authority slice.

## Watchouts

RHS `fadd`, `fmul`, and now `fsub` are all admitted DirectScalar floating RHS
authority rows, so neighboring negative tests for those rows must use a still
unsupported operator such as `fdiv`, not another admitted RHS row.

## Proof

Passed the delegated proof:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_lir_to_bir_interface$'; } > test_after.log 2>&1`.

Supervisor broader backend proof also passed:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; }`
with `6/6` backend tests passing.
