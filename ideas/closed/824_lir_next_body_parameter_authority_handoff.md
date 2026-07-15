# LIR Next Body-Parameter Authority Handoff

Status: Closed (capability complete)
Type: bounded LIR producer/schema/verifier authority publication
Predecessor: `ideas/open/734_lir_to_new_bir_container_completeness.md` post-Step 7.36
Consumer: `ideas/open/734_lir_to_new_bir_container_completeness.md`

## Goal

Trace, publish, verify, and hand off exactly one next valid current-LIR
function-body parameter-use authority row for a later lossless typed Raw-BIR
receipt.

## Why This Exists

734 accepted the selected DirectScalar binary-RHS receipt in `c87976453`, but
its coverage matrix remains incomplete. No next receiver-ready parameter row
is evidenced. This idea owns only the producer-side selection and structured
authority handoff needed before 734 can resume.

## In Scope

- Trace one candidate native body-parameter use and prove whether it has a
  current-function structured identity, owner, type, ABI, and exact operand
  relation sufficient for one receiver row.
- If that candidate is valid, publish and verify the minimum native structured
  carrier, including malformed, foreign, duplicate, role/operand, owner, ABI,
  and type-incoherent rejection as applicable.
- Record focused same-feature positive/negative proof and an exact one-row
  handoff to 734.

## Out Of Scope

- Raw-BIR structs, builders, importers, receivers, or broad scalar/parameter
  admission.
- Reopening accepted direct-pointer, DirectScalar binary-LHS, or DirectScalar
  binary-RHS rows.
- Memory/VA, aggregate/vector, module/type/global/metadata, other
  instruction/terminator, inline-assembly, ABI redesign, or text-derived
  identity.

## Acceptance Criteria

- Exactly one selected parameter-use row has an explicit checked native
  authority contract and focused positive/negative proof, with a receiver
  handoff that names every required field and transactional rejection boundary.
- If no candidate is receiver-ready, the evidence names the first missing
  authority prerequisite and routes it as a separately scoped successor;
  nonselected forms remain fail closed.
- The outcome tells 734 exactly whether and where to resume, without claiming
  Raw-BIR receipt or source-wide coverage completion.

## Reviewer Reject Signals

- Reject presentation-derived identity from parameter spelling, rendered
  operands, signatures, diagnostics, or testcase names.
- Reject generic scalar/parameter admission, ABI broadening, Raw-BIR/importer
  edits, or a combined body-parameter sweep claimed as one row.
- Reject expectation downgrades, weaker verifier/test contracts, named-case
  shortcuts, or retaining the old unstructured failure behind a renamed carrier.

## Closure Record

Disposition: capability complete for this bounded producer/schema/verifier
handoff. The accepted implementation is `fac485148`.

The selected and only authorized receiver row is an optional
`LirRet.return_value_parameter_authority` for an unchanged current-function
DirectScalar parameter returned through the exact SSA return operand. The
carrier contains the value identity, parameter index, `LirTypeRef`, owning
`LinkNameId`, `LirNativeBodyParameterAbi::DirectScalar`, and `ReturnValue`
role. Verification requires exactly one matching native definition and rejects
missing carrier for the selected form, malformed, foreign, duplicate,
owner/index/type/ABI/role-incoherent authority, return-operand mismatch, and
signature-return mismatch transactionally. Nonselected return forms synthesize
no carrier and remain outside this row.

Accepted proof: fresh
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`
passed 6/6, and the matching before/after regression guard passed 6/6 on both
sides without regression.

Exact consumer return point: reactivate
`ideas/open/734_lir_to_new_bir_container_completeness.md` at **Step 7.37 -
Receive the one 824-authorized DirectScalar return-value parameter authority
row**. Add only this tuple's typed Raw-BIR return destination, importer
dispatch, reachable verification, and transactional positive/malformed-
authority coverage. No Raw-BIR/importer/receiver work landed in 824; do not
repeat Steps 1 through 7.36 or receive another parameter form.
