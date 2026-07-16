Status: Active
Source Idea Path: ideas/open/863_lir_next_non_body_parameter_authority_handoff.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Select and publish one next non-body-parameter authority row

# Current Packet

## Just Finished
Step 1 selected and published exactly one next current-LIR non-body-parameter
authority row: direct zero-argument scalar floating `LirCallOp` result
authority for the selected result consumed as the LHS of the downstream
floating binary operation. The producer now carries
`LirDirectZeroArgScalarFloatingCallAuthority` with native result `LirValueId`,
current-function owner `LinkNameId`, direct callee `LinkNameId`, exact
floating `LirTypeRef`, and explicit `ResultIntoFloatingBinaryLhs` role.
Verifier and focused frontend LIR coverage now reject absent, stale result,
foreign owner, callee-incoherent, type-incoherent, role-incoherent, and
consumer-incoherent forms.

## Suggested Next
Return to 734 with one receiver packet for the selected direct zero-argument
scalar floating call-result row. Receive only the structured result/owner/
callee/return-type/role tuple into typed Raw BIR and keep all other
nonselected current-LIR rows fail-closed.

## Watchouts
This packet selected only the direct zero-argument scalar floating result-call
row. It did not authorize Raw-BIR receiver edits, body-parameter rows,
multiple call families, nonzero-argument floating calls, indirect calls,
aggregate/vector/memory/VA/CFG/PHI/inline-assembly rows, or presentation-based
recovery.

## Proof
Passed. Proof log: `test_after.log`.

Command:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_lir_(function_signature_type_ref|extern_decl_type_ref|global_type_ref|call_type_ref)$'; } > test_after.log 2>&1`

Supervisor full-suite proof also passed:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure; } > test_after.log 2>&1`
with `3038/3038` tests passing.
