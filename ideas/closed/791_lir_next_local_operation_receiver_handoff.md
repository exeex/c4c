# Next LIR Local-Operation Receiver Handoff

Status: Closed (capability complete)
Type: bounded LIR local-operation authority handoff
Predecessor: `ideas/open/734_lir_to_new_bir_container_completeness.md`
Consumer: `ideas/open/734_lir_to_new_bir_container_completeness.md`

## Goal

Authorize exactly one next valid local-operation Raw-BIR receiver row after
734's accepted direct local-scalar store receipt, with complete native
current-function authority.

## Why This Exists

Closed 790 authorized only the selected declaration store, which 734 accepted
in `f5cda70ee`. The remaining local GEP/VLA, named/local-temporary, and
nonselected-store routes have no exact receiver-ready handoff. 734 cannot
choose a row from local spelling, representative coverage, or rendered
operands; producer-side selection and verifier admission must name the next
typed contract first.

## In Scope

- Inspect existing native local-object authority routes after the accepted
  direct local-scalar store.
- Select and publish exactly one earliest admissible local GEP, VLA-lifetime,
  named/local-temporary, or other local-operation row only when native fields
  prove its result/use, pointer/object ownership, type coherence, and liveness
  without text recovery.
- Add only the minimum producer/schema and verifier admission required for
  that one selected row.
- Prove current-function pointer-definition/object ownership, type coherence,
  and liveness with nearby positive and malformed-authority coverage.
- Record a precise handoff to 734: selected variant, native fields,
  guarantees, rejected forms, focused proof, and exact Step 7.29 receiver
  action.

## Out Of Scope

- Raw-BIR containers, importer, receiver verifier, target lowering, MIR, and
  emission.
- More than one local-operation row; broad local store/GEP/VLA conversion;
  memory/va, aggregate/vector, body parameters, CFG/PHI, and later families.
- Recovering semantics from local names, `%t`, formatted operands, printer
  output, LLVM text, or testcase identity.

## Acceptance Criteria

- Exactly one post-store local-operation row has a documented native typed
  authority contract suitable for 734 Step 7.29 consumption.
- Missing, invalid, foreign, object/pointer/type-incoherent, dead, or
  display-derived authority rejects before downstream use.
- Focused same-feature producer positive and negative proof covers the
  selected row, while all nonselected local rows remain fail closed.
- The closure handoff lets 734 resume without re-deriving a row from
  presentation or repeating its accepted alloca, load, or store receipts.

## Reviewer Reject Signals

- Reject selection from a local name, `%t`, rendered operand, LLVM text, or
  named testcase rather than native structured authority.
- Reject a broad local-operation conversion or any Raw-BIR/importer/target
  lowering change claimed as this one bounded handoff.
- Reject verifier weakening, expectation downgrades, or compatibility text as
  a substitute for current-function object/pointer/type/lifetime facts.
- Reject a handoff that does not name exactly one 734 receiver row, its typed
  fields, rejected forms, focused proof, and Step 7.29 return action.

## Closure Record

Capability complete. Commit `ea579c648` selected exactly the direct
static-local-array `LirGepOp` with a valid current-function result, native SSA
base pointer, one native `i64` immediate index, matching element type, and
checked live local-object authority. The verifier rejects missing admission,
malformed result/base/index, foreign or incoherent object/owner, type mismatch,
and dead authority before downstream use; display spelling remains
nonsemantic. The handoff at
`docs/lir_local_operation_authority/handoff_to_734.md` authorizes 734 Step
7.29 only.

Supervisor acceptance proof is a fresh build plus focused
`^frontend_lir_call_type_ref$` 1/1, a matching non-decreasing canonical
before/after 1/1 guard, broader `^frontend_cxx_` 1/1 and `^backend_` 5/5.
Direct supervisor review found no material issue and the hook review state is
cleared. All nonselected local rows remain fail closed.

Exact return action: reactivate 734 and repair its runbook for only
`Step 7.29 - Receive the selected direct static-local-array LirGepOp
authority`. Consume only the documented result, element type, base pointer,
immediate index, and local-object authority; do not repeat accepted alloca,
local-load, or declaration-store receipts, or widen to any other family.
