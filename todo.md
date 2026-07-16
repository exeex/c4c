Status: Active
Source Idea Path: ideas/open/829_lir_next_body_parameter_authority_handoff.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Publish And Verify The Selected Authority

# Current Packet

## Just Finished

Activated 829 at its recorded return point after closed 830 accepted the
direct-call argument-1 structured identity/type prerequisite. Step 1 trace
remains accepted historical work from `78b17b3d3`.

## Suggested Next

Implement Step 2: publish exactly one DirectScalar current-function
body-parameter authority row for `LirCallOp.structured_args[1]` of a direct,
non-variadic, specified call. Add malformed-authority coverage and run focused
`frontend_lir_call_type_ref` proof.

## Watchouts

- Do not edit Raw-BIR/importer/receiver code or 734 receipt.
- Do not select another parameter row, argument index, generic call form,
  variadic/indirect/unspecified call, or ABI conversion.
- Do not recover identity from text, signatures, operands, printer output, or
  diagnostics.
- 830 only provides the argument-1 identity/type prerequisite; 829 must still
  verify the body-parameter authority tuple independently.

## Proof

Passed focused proof:
`( cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_lir_call_type_ref$' ) > test_after.log 2>&1`.
Matching before/after `frontend_lir_call_type_ref` regression guard passed.
