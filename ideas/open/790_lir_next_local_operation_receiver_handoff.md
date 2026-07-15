# Next LIR Local-Operation Receiver Handoff

Status: Open
Type: bounded LIR local-operation authority handoff
Predecessor: `ideas/open/734_lir_to_new_bir_container_completeness.md`
Consumer: `ideas/open/734_lir_to_new_bir_container_completeness.md`

## Goal

Authorize exactly one next valid post-local-scalar-load local-operation
Raw-BIR receiver row with complete native current-function authority.

## Why This Exists

Closed 789 selected only the first local load row, which 734 accepted in
`eabf7a3b8`. The next local store/GEP/VLA family has no exact receiver-ready
handoff. 734 must not choose a row from local names, presentation text, or
representative coverage.

## In Scope

- Inspect the existing native local-object authority routes after the accepted
  direct non-array/non-VLA local-scalar load.
- Select and publish exactly one earliest valid local store, GEP, or VLA
  lifetime row, only if native fields can prove its result/use, pointer/object
  ownership, type coherence, and liveness without text recovery.
- Add the minimum producer/schema and verifier admission required for that one
  row, with focused positive and malformed authority coverage.
- Record a precise handoff to 734 naming the selected variant, native fields,
  guarantees, rejected forms, proof, and exact Step 7.28 receiver action.

## Out Of Scope

- Raw-BIR containers, importer, receiver verifier, target lowering, MIR, and
  emission.
- More than one local store/GEP/VLA row; later local loads, named or temporary
  variants, memory/va, aggregate/vector, body parameters, CFG/PHI, and all
  other families.
- Recovering semantics from local names, `%t`, formatted operands, printers,
  LLVM text, or testcase identity.

## Acceptance Criteria

- One exact next local-operation row has a documented native typed contract
  sufficient for 734 to consume at Step 7.28.
- Missing, invalid, foreign, object/pointer/type-incoherent, or dead native
  authority rejects before downstream use. Display spelling is ignored: it is
  neither recovered nor used as a validity check.
- Focused same-feature producer positive and negative proof passes, and the
  handoff preserves all other rows as fail-closed.

## Reviewer Reject Signals

- Reject selecting the row from a local name, `%t`, rendered operand, LLVM
  text, or testcase shape rather than native structured authority.
- Reject a broad local store/GEP/VLA conversion, or Raw-BIR/importer work,
  claimed as this one-row handoff.
- Reject verifier weakening, expectation downgrades, or compatibility text as
  a substitute for current-function result/use, pointer/object/type, and
  liveness facts.
- Reject a handoff that does not name exactly one 734 receiver row, its native
  fields, rejected forms, focused proof, and Step 7.28 return action.
