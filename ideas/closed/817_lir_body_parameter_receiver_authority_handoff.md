# LIR Body-Parameter Receiver Authority Handoff

Status: Open
Type: bounded LIR producer/schema/verifier authority publication
Predecessor: post-Step 7.33 of `ideas/open/734_lir_to_new_bir_container_completeness.md`
Consumer: `ideas/open/734_lir_to_new_bir_container_completeness.md`

## Goal

Select and publish exactly one function-body parameter-use row with native, current-function semantic authority so 734 can later receive that one row into typed Raw-BIR without using names, declaration-only facts, or rendered text.

## In Scope

- Trace the producer, schema, and verifier paths to select one receiver-ready parameter-use row only when its value ID, type, ownership, and required ABI classification are native and checkable.
- Publish and verify only that selected contract, with malformed, missing, foreign, duplicate, and type-incoherent rejection as applicable.
- Record the exact 734 handoff: allowed fields, rejected forms, focused proof, and one receiver return point.

## Out Of Scope

- Raw-BIR/importer/verifier work or a 734 receiver change.
- Declaration-only parameter publication, all parameter shapes, broad ABI conversion, aggregate/vector, memory/VA, call, CFG, module, or inline-asm work.
- Recovery from parameter spelling, signature rendering, diagnostics, operands, or testcase text.

## Acceptance Criteria

- Exactly one body-parameter use row has a checked native authority contract and focused positive/malformed proof.
- All nonselected parameter forms remain classified or fail closed, and the handoff permits only that row's later 734 receipt.
- A fresh build and selected focused proof pass before handoff acceptance.

## Reviewer Reject Signals

- Declaration facts, names, rendered signatures, operands, or diagnostics establish body-use authority.
- More than one parameter family, Raw-BIR/receiver work, broad ABI work, or unrelated residual work is absorbed.
- Tests weaken contracts, special-case a testcase, or accept missing, foreign, duplicate, or type-incoherent authority.
- The handoff omits ownership, exact fields, rejected forms, focused proof, or the 734 return point.

## Closure Record

Disposition: capability complete.

Commit `613f947b5` publishes and verifies exactly one native current-function
body-parameter authority row: the direct non-expanded pointer parameter used
as `LirGepOp.ptr`, the typed-GEP base for `p[0]`. Its handoff fields are exactly
`LirValueId`, parameter index, pointer `LirTypeRef`, current `LinkNameId`
owner, and explicit `LirNativeBodyParameterAbi::DirectPointer`.

Scalar, byval/aggregate, HFA/vector, array, variadic, and all other parameter
forms are rejected and fail closed. Parameter spelling, signature text, raw
operands, and diagnostics are not authority.

Supervisor acceptance proof: fresh `cmake --build --preset default`; focused
`ctest --test-dir build -j --output-on-failure -R '^backend_lir_selected_pointer_authority$'`;
and a matching before/after regression guard with a non-decreasing 1/1 pass
count. The only 734 return point is a later single Raw-BIR receipt for that
one structured row; this idea performed no receiver work.
