# LIR Vaarg PHI Input Identity Runbook

Status: Active
Source Idea: ideas/open/782_lir_vaarg_phi_input_result_identity_publication.md
Resumed after: closed `ideas/closed/783_lir_native_vaarg_operand_result_seam_decomposition.md`.

## Purpose

Give 751 native value authority for the intermediate vaarg values that already
feed its three existing PHI constructors, without changing the PHI consumer.

## Goal

Publish native, current-function typed result/value identity for every existing
vaarg PHI helper input before its compatibility spelling is consumed.

## Core Rule

The helper-input field is semantic authority. Textual result names, labels,
rendered LLVM, instruction order, and testcase identity are never recovery
inputs; closed 775's `LirVaArgOp.result` is not a substitute for it.

## Read First

- `ideas/open/782_lir_vaarg_phi_input_result_identity_publication.md`
- `ideas/closed/783_lir_native_vaarg_operand_result_seam_decomposition.md`
- `ideas/open/751_lir_phi_incoming_value_and_predecessor_identity.md`
- closed `ideas/closed/775_lir_phi_producer_helper_result_identity.md`
- `src/codegen/lir/hir_to_lir/call/vaarg.cpp`
- `src/codegen/lir/hir_to_lir/call/vaarg_amd64.cpp`

## Non-Goals

- Do not change `LirPhiOp`, its verifier, predecessor/edge authority, CFG,
  Raw-BIR/importer, backend, target lowering, MIR, or emission.
- Do not broaden into other vaarg paths, generic expression APIs, side tables,
  result-name maps, or text recovery.

## Execution Rules

1. Use 783's accepted native source-to-immediate-consumer contracts as upstream
   facts; do not reopen its carrier or value-only PHI transport decision.
2. Keep native helper-input result fields current-function-owned and adjacent
   to the operations that define the PHI input values.
3. Preserve raw spelling only as rendering compatibility after authority is
   available.
4. Fail closed for missing, invalid, duplicate, and foreign authority.
5. Cover all three existing constructors: AArch64 GP, AArch64 FP, and AMD64.
6. Do not assert PHI carrier/verifier completion; the only downstream handoff
   is the named input fields needed when 751 resumes Step 1.

## Ordered Steps

### Step 1 - Publish native vaarg PHI-helper input fields

Goal: make each existing vaarg PHI input retain its native typed,
current-function result/value identity before the PHI construction site.

Primary targets:

- `src/codegen/lir/hir_to_lir/call/vaarg.cpp`
- `src/codegen/lir/hir_to_lir/call/vaarg_amd64.cpp`
- the smallest shared LIR helper-result seam required by those inputs

Actions:

- trace every raw input to the AArch64 GP, AArch64 FP, and AMD64 vaarg PHI
  constructors to its defining helper operation using 783's accepted contracts;
- add or forward only native typed fields needed for those inputs, retaining
  compatibility spelling without recovering from it; and
- reject absent, invalid, duplicate, and foreign current-function authority at
  the existing producer-verification seam.

Completion check:

- every existing vaarg PHI input has a native current-function typed helper
  result field; no `LirPhiOp`, predecessor/edge, CFG, or downstream change
  occurs.

### Step 2 - Prove the bounded handoff and return to 751

Goal: demonstrate structural authority for all three vaarg PHI constructors
and publish the exact consumer handoff.

Actions:

- add nearby positive and malformed coverage for AArch64 GP, AArch64 FP, and
  AMD64 input chains;
- run a fresh build and the focused vaarg/result-authority proof; and
- record the accepted field names, covered constructors, proof, and commit for
  751 without claiming its PHI carrier/verifier work.

Completion check:

- the supervisor accepts fresh focused proof for all three constructors and
  751 can be reactivated at its recorded Step 1.
