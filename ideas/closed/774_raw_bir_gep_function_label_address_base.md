# Raw-BIR GEP Function-Owned Label-Address Base

Status: Open (active blocker for
`ideas/open/773_lir_gep_direct_label_address_constant_contract.md`, Step 2)
Type: bounded Raw-BIR schema, builder, and lowering contract transition
Blocked parent: `ideas/open/773_lir_gep_direct_label_address_constant_contract.md`, Step 2

## Goal

Evolve Raw-BIR GEP base representation so a verified, function-owned direct
label-address can be carried as structured authority from LIR lowering, rather
than being forced into the existing global-array-only `GlobalObjectId` base.

## Why This Exists

773 Step 1 accepted the exact typed LIR direct-label-address GEP base, but its
Step 2 cannot reach lowering. `GetElementPtrSpec` and `GetElementPtrNode` in
`src/backend/bir/core/{builder.hpp,ir.hpp,builder.cpp}` encode `base` strictly
as `GlobalObjectId`, and the builder insists on the exact global array object.
The verified source is instead a current-function `ValueId` /
`LabelAddressConstant`; `lir_to_bir.cpp` consequently rejects it before
lowering. This is a Raw-BIR core representation boundary, not a printer-only
or 773 forwarding change.

## In Scope

- define a structured Raw-BIR GEP-base representation that distinguishes the
  existing global-object base from the verified function-owned direct
  label-address authority
- update Raw-BIR GEP node/spec storage, builder validation, and downstream
  Raw-BIR lowering consumers so that representation preserves the authority
  without text recovery or fabricated values
- retain the existing global-array GEP-base contract and reject malformed,
  foreign, missing, non-label-address, or otherwise unsupported function-owned
  base identities
- add focused positive and malformed tests at the Raw-BIR builder and lowering
  boundary
- return a precise compatibility contract enabling 773 Step 2 to add its
  printer receipt and typed-direct-label LIR-to-BIR dispatch

## Out Of Scope

- generic Raw-BIR redesign or widening of unrelated BIR operands
- raw text, label spelling, rendered output, or importer-based identity
  recovery
- synthetic SSA, synthetic globals, or coercing a function-owned label address
  into a global object
- LIR-to-BIR dispatch or LIR printer changes, 773's printer/forwarding
  completion, or 772's
  `emit_indexed_gep` repair and `pr70460` proof
- Raw-BIR importer work, 734 work, computed-goto carrier changes, and any
  generic computed-goto support

## Acceptance Criteria

- Raw-BIR can represent and build the one verified function-owned direct
  label-address GEP-base form as structured authority while preserving the
  existing global-object route.
- Builder and lowering reject missing, foreign, arbitrary, non-label-address,
  malformed, or unsupported base forms fail closed; no textual or synthetic
  fallback exists.
- Focused positive coverage reaches the new structured path, and focused
  malformed coverage exercises its relevant rejection boundaries as well as
  preserving the global-array contract.
- A fresh build and selected focused Raw-BIR proof pass. The
  supervisor receives exact proof and a return handoff to resume 773 Step 2;
  this blocker does not claim the printer, 773 completion, 772 forwarding, or
  baseline repair.

## Reviewer Reject Signals

- Reject a named-testcase branch, expectation downgrade, failure exclusion, or
  classification-only change claimed as structured GEP-base support.
- Reject retaining the old `GlobalObjectId`-only failure behind a new wrapper,
  or accepting the form through raw text, label spelling, printer output, or
  rendered LLVM recovery.
- Reject synthetic SSA/global manufacture or a generic value-ID escape hatch
  that bypasses explicit function ownership and direct-label-address identity.
- Reject builder relaxation that accepts arbitrary, foreign, missing, or
  non-label-address function values, or that weakens the existing global-array
  GEP-base contract.
- Reject a generic BIR rewrite, importer/734/computed-goto work, LIR-to-BIR
  dispatch or printer work, 773 forwarding changes, or the 772 production
  repair in this blocker.
- Reject focused-positive-only acceptance without malformed contract coverage
  and a fresh build-backed Raw-BIR proof.

## Closure Record

Disposition: capability complete. This bounded Raw-BIR blocker is accepted and
returns control to its parent, 773 Step 2.

- Accepted implementation: `c64b78c48` (`backend: model Raw-BIR label-address
  GEP bases`) introduced the explicit global-or-function-label authority, with
  focused positive and malformed boundary coverage. `97121c359` (`backend:
  verify typed Raw-BIR GEP bases`) made Raw-BIR verification consume that typed
  authority directly, retaining the global-array contract and rejecting invalid
  label bases.
- Accepted proof: fresh `cmake --build --preset default` and
  `ctest --test-dir build -j --output-on-failure -R
  '^backend_lir_to_bir_interface$'` passed. The earlier matched `^backend_`
  before/after guards each passed 5/5.
- Baseline boundary: `test_baseline.new.log` remains rejected at 3037 total / 1
  failure (`llvm_gcc_c_torture_src_pr70460_c`). It is the separately scoped 772
  `emit_indexed_gep` / pr70460 forwarding issue, is not repaired or accepted
  here, and must not be refreshed by this closure.
- Parent return contract: 773 resumes its preserved Step 2 only to add the LIR
  printer receipt and typed-direct-label LIR-to-BIR dispatch using this Raw-BIR
  authority, with nearby malformed coverage. It must not absorb 772 forwarding
  or pr70460 work; those remain for 773's later handoff to 772 Step 1.
