# Typed Direct Label-Address Constant Contract for LIR GEP Pointers

Status: Open (active blocker for
`ideas/open/772_lir_gep_pointer_authority_pr70460.md`)
Type: bounded LIR verifier/printer/backend contract transition
Blocked parent: `ideas/open/772_lir_gep_pointer_authority_pr70460.md`, Step 1

## Goal

Allow a verified, current-function direct label-address
`LirOperand::DirectConstant(LirValueId)` to serve directly as `LirGepOp.ptr`
through the LIR verifier, printer, and backend/lowering contracts, while
retaining fail-closed validation for every other pointer authority form.

## Why This Exists

The parent route located its first production loss at
`StmtEmitter::emit_indexed_gep(FnCtx&, const LirOperand&, ...)` in
`src/codegen/lir/hir_to_lir/lvalue.cpp:958`: forwarding a direct constant by
`.str()` produces `RawText(\"\")`. That seam cannot safely retain the typed
constant today. `verify_authoritative_gep` accepts only `Global` or `SsaValue`;
generic pointer verification excludes `DirectConstant`; the printer requires
an SSA/global GEP base; and backend lowering makes the same assumption.
Converting the function-owned direct label-address constant into synthetic SSA
fails current-function definition verification. This is a required contract
transition, not a repair permitted by 772's explicit verifier/printer/backend
non-goal.

## In Scope

- define the exact `LirGepOp.ptr` admissibility rule for a typed
  `LirOperand::DirectConstant(LirValueId)` that resolves through the current
  function's `direct_label_address_constants` ownership
- update only the verifier checks needed to validate presence, value ID,
  current-function ownership, label-address identity, pointer suitability, and
  any existing display-mirror obligations for that operand form
- make LIR printing and backend/lowering consume the validated typed direct
  label-address constant as a GEP base without text recovery or synthetic SSA
- add focused positive and malformed contract coverage near the verifier,
  printer, and lowering surfaces
- provide the parent return contract: once this idea is accepted, 772 resumes
  at Step 1 and forwards the typed direct constant at its structured
  `emit_indexed_gep` seam

## Out Of Scope

- generic GEP redesign or widening unrelated pointer operand forms
- synthetic SSA, `select`, `gep`, `bitcast`, PHI, alloca/load, or dummy value
  creation to carry a direct label address
- label, spelling, printer-output, rendered-LLVM, or `RawText` identity
  recovery
- the `pr70460` `emit_indexed_gep` forwarding-seam edit itself
- computed-goto carrier changes, Raw-BIR/importer work, 734 work, or changes
  to the accepted 764/770/771 direct-constant contracts outside GEP use

## Acceptance Criteria

- A GEP accepts a `DirectConstant(LirValueId)` only when it denotes a valid
  current-function direct label-address constant with the required pointer
  authority; invalid, foreign, missing, non-label-address, non-pointer, or
  display-inconsistent forms fail closed.
- The printer and backend/lowering render/lower that validated operand through
  its typed direct-constant authority, with no SSA fabrication or text-based
  fallback.
- Focused positive coverage reaches the direct-label-address GEP contract and
  focused malformed coverage proves each relevant rejection boundary.
- A fresh build and the selected focused verifier/printer/lowering proof pass;
  the supervisor receives exact proof and parent-resumption handoff. No
  `pr70460` production forwarding change is claimed by this idea.

## Reviewer Reject Signals

- Reject a named-testcase branch, expectation downgrade, failure exclusion, or
  test-only classification change claimed as this contract transition.
- Reject recovery of the operand from raw text, label spelling, printer output,
  or rendered LLVM, including a newly named abstraction that preserves the
  old empty-pointer route.
- Reject synthetic SSA or other generated instruction/value IDs used to evade
  function-owned direct-constant verification.
- Reject verifier relaxation that admits arbitrary `DirectConstant`, foreign
  IDs, non-label-address constants, or unvalidated pointer operands.
- Reject a generic GEP/backend rewrite, a computed-goto carrier change,
  Raw-BIR/importer/734 work, or the 772 `emit_indexed_gep` repair in this
  blocker.
- Reject focused-positive-only acceptance without malformed rejection coverage
  and a fresh build-backed printer/lowering proof.

## Resumption Record

Status: Interrupted by separately scoped active blocker
`ideas/open/774_raw_bir_gep_function_label_address_base.md`.

- Last accepted progress: Step 1, `Specify and verify the typed
  direct-label-address GEP base`, is accepted in `a4415f99c`
  (`lir: verify direct label constants as GEP bases`). It admitted only the
  current-function typed `DirectConstant(LirValueId)` direct label-address
  GEP base at the verifier boundary. Fresh build plus
  `^backend_lir_to_bir_interface$` passed for that accepted slice.
- Interrupted step: Step 2, `Carry the validated form through printer and
  backend/lowering`. No Step 2 code, focused proof, or acceptance exists.
- Blocker: Raw-BIR's `GetElementPtrSpec` / `GetElementPtrNode` schema and
  builder encode `base` strictly as `GlobalObjectId`; the builder requires the
  exact global array object. The verified direct label address instead resolves
  as a current-function `ValueId` / `LabelAddressConstant`, and
  `lir_to_bir.cpp` rejects it before lowering. Carrying this structured
  identity therefore requires a Raw-BIR core schema/builder boundary change,
  outside this idea's explicit non-goals and Step 2 packet.
- Exact return point: after 774 is accepted, resume this idea at Step 2. Add
  the printer receipt and LIR-to-BIR dispatch for the already-verified typed
  direct label address, using the new structured Raw-BIR authority; retain
  malformed-boundary coverage. Then execute Step 3 and hand 772 back to its
  Step 1 structured `emit_indexed_gep` forwarding repair.
- Remaining acceptance: printer and backend/lowering proof for this typed
  form, followed by Step 3 fresh focused proof and the precise handoff to 772.
  The rejected baseline remains rejected until those steps complete and 772
  repairs its forwarding seam so `llvm_gcc_c_torture_src_pr70460_c` passes.
