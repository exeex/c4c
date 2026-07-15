# Next LIR Local-Operation Receiver Handoff

Status: Open (active)
Type: bounded LIR local-operation authority handoff
Predecessor: `ideas/open/734_lir_to_new_bir_container_completeness.md`
Consumer: `ideas/open/734_lir_to_new_bir_container_completeness.md`

## Goal

Authorize exactly one next valid local-operation Raw-BIR receiver row after
734's accepted direct static-local-array GEP receipt, with complete native
current-function authority.

## Why This Exists

Closed 791 authorized only the direct static-local-array `LirGepOp`, which 734
accepted in `4ab2deb7e`. Remaining local/VLA, named/local-temporary, and
nonselected store/GEP routes have no exact receiver-ready handoff. 734 cannot
choose a row from local spelling, representative coverage, or rendered
operands; producer-side selection and verifier admission must name the next
typed contract first.

## In Scope

- Inspect existing native local-object authority routes after the accepted
  local-array GEP.
- Select and publish exactly one earliest admissible local GEP, VLA-lifetime,
  named/local-temporary, or other local-operation row only when native fields
  prove its result/use, pointer/object ownership, type coherence, and liveness
  without text recovery.
- Add only the minimum producer/schema and verifier admission required for
  that one selected row.
- Prove current-function pointer-definition/object ownership, type coherence,
  and liveness with nearby positive and malformed-authority coverage.
- Record a precise handoff to 734: selected variant, native fields,
  guarantees, rejected forms, focused proof, and exact Step 7.30 receiver
  action.

## Out Of Scope

- Raw-BIR containers, importer, receiver verifier, target lowering, MIR, and
  emission.
- More than one local-operation row; broad local store/GEP/VLA conversion;
  memory/va, aggregate/vector, body parameters, CFG/PHI, and later families.
- Recovering semantics from local names, `%t`, formatted operands, printer
  output, LLVM text, or testcase identity.

## Acceptance Criteria

- Exactly one post-GEP local-operation row has a documented native typed
  authority contract suitable for 734 Step 7.30 consumption.
- Missing, invalid, foreign, object/pointer/type-incoherent, dead, or
  display-derived authority rejects before downstream use.
- Focused same-feature producer positive and negative proof covers the
  selected row, while all nonselected local rows remain fail closed.
- The closure handoff lets 734 resume without re-deriving a row from
  presentation or repeating accepted alloca, load, store, or local-array GEP
  receipts.

## Reviewer Reject Signals

- Reject selection from a local name, `%t`, rendered operand, LLVM text, or
  named testcase rather than native structured authority.
- Reject a broad local-operation conversion or any Raw-BIR/importer/target
  lowering change claimed as this one bounded handoff.
- Reject verifier weakening, expectation downgrades, or compatibility text as
  a substitute for current-function object/pointer/type/lifetime facts.
- Reject a handoff that does not name exactly one 734 receiver row, its typed
  fields, rejected forms, focused proof, and Step 7.30 return action.
