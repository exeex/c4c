# LIR Local Operation Receiver Handoff Completion

Status: Closed
Type: bounded LIR local-operation authority handoff
Predecessor: `ideas/closed/752_lir_local_object_pointer_authority_convergence.md`
Consumer: `ideas/open/734_lir_to_new_bir_container_completeness.md`

## Goal

Authorize exactly one first post-alloca local-operation Raw-BIR receiver row
with complete current-function typed authority, without widening closed 752's
sole alloca handoff or changing Raw-BIR.

## Why This Exists

Closed 752 proves a shared local-object/pointer authority model and explicitly
orders alloca before local load/store/GEP and VLA lifetime routes. Its closure
authorizes only the selected hoisted alloca row for 734. After 734 accepted
that row in `2cce9da69`, no exact next receiver-ready local row is named.
734 cannot select one from spelling, representative coverage, or a rendered
operand; producer-side selection and verifier admission must state the next
typed contract first.

## In Scope

- Inspect closed-752 selected local load/store/GEP and VLA lifetime authority
  routes and choose exactly one earliest admissible post-alloca row.
- Publish or tighten only native typed fields and verifier admission needed for
  that one row when the existing 752 carrier is insufficient.
- Prove current-function pointer-definition/object ownership, type coherence,
  and liveness for the selected row, with nearby positive and malformed cases.
- Record a precise 734 handoff: selected variant, fields, guarantees, rejected
  forms, proof, and exact one-row return action.

## Out Of Scope

- Raw-BIR containers, importer, receiver verification, target lowering, MIR,
  or emission.
- More than one local load/store/GEP/VLA row; named/local-temporary variants,
  memory/va, aggregate/vector, body parameters, CFG/PHI, and later families.
- Recovering semantics from local names, `%t`, formatted operands, printer
  output, LLVM text, or testcase identity.

## Acceptance Criteria

- Exactly one post-alloca local-operation row has a documented native typed
  authority contract suitable for 734 consumption.
- Missing, invalid, foreign, type-mismatched, dead, or display-mismatched
  authority rejects before downstream use.
- Focused same-feature positive and negative proof covers the selected row.
- The closure handoff lets 734 resume at Step 7.27 without re-deriving a row
  from presentation or repeating accepted alloca receipt.

## Reviewer Reject Signals

- Reject selecting a receiver row from a local name, `%t`, rendered operand,
  LLVM text, or named testcase rather than native structured authority.
- Reject a broad local load/store/GEP/VLA conversion claimed as one bounded
  handoff, or any Raw-BIR/importer/target-lowering change.
- Reject verifier weakening, expectation downgrades, or compatibility text as
  a substitute for current-function object/pointer/lifetime facts.
- Reject a handoff that does not name one exact 734 receiver row and its typed
  fields, rejected forms, and proof.

## Closure Record

Capability complete. The exact selected direct non-array/non-VLA local-scalar
`LirLoadOp` contract is published in
`docs/lir_local_operation_authority/handoff_to_734.md`. Commit `d48236bd9`
selects the producer and `d290ffc21` enforces native result admission,
pointer-definition binding, and pointee/load-type coherence. Its focused fresh
build plus `^frontend_lir_call_type_ref$` proof passed 1/1; the matching
canonical before/after regression guard passed non-decreasing. 734 resumes at
Step 7.27 for only the documented receiver packet; every later local row stays
out of scope.
