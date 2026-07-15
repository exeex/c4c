# LIR DirectScalar Body-Parameter Producer/Verifier Publication Runbook

Status: Active
Source Idea: ideas/open/820_lir_directscalar_parameter_producer_verifier_publication.md
Switched from: 734 Step 7.35 (unaccepted receiver work retained outside this
runbook)

## Purpose

Repair the narrow LIR producer/verifier publication defect that prevents the
existing `ull` DirectScalar parameter path from reaching 734's selected
receiver route.

## Core Rule

Publish only structured native current-function DirectScalar identity and type.
Do not derive authority from presentation fields and do not change Raw-BIR or
admit generic scalar parameter receipt.

## Read First

- `ideas/open/820_lir_directscalar_parameter_producer_verifier_publication.md`
- `ideas/open/734_lir_to_new_bir_container_completeness.md` (latest resumption
  record)
- `src/codegen/lir/verify.cpp` and the producer site for the existing `ull`
  native DirectScalar body parameter

## Non-Goals

- Raw-BIR/importer work, generic scalar parameters, other ABI forms, and
  broader DirectScalar authority redesign.

## Ordered Steps

### Step 1 - Trace the existing `ull` DirectScalar authority seam

Goal: identify the exact producer/emitter path and verifier inputs for the
existing `ull` native DirectScalar parameter.

Actions:

- establish where current-function parameter identity and type are lost;
- confirm the repair can stay limited to the existing `ull` shape;
- stop and return a scope blocker if publication necessarily requires a
  broader authority family.

Completion check: one typed producer/verifier seam is named without relying on
textual fields or expanding authority scope.

### Step 2 - Publish and verify the typed DirectScalar authority

Goal: make the existing `ull` path satisfy the native body-parameter contract
while malformed DirectScalar authority remains rejected.

Actions:

- implement the minimal producer/emitter and LIR verifier correction;
- add focused positive and malformed-authority coverage;
- keep unselected DirectScalar rows outside Raw-BIR receipt.

Completion check: valid `ull` authority verifies; missing, foreign, and
type-incoherent authority fails closed.

### Step 3 - Prove the boundary and record the 734 handoff

Goal: demonstrate that the prior external failure no longer occurs at the LIR
verifier boundary and return only the selected receiver authorization.

Actions:

- run a fresh build and focused producer/verifier proof;
- run `ctest --test-dir build --output-on-failure -R
  '^llvm_gcc_c_torture_src_20041011_1_c$'`;
- run the supervisor-selected matching regression/broader proof;
- record that 734 resumes solely at Step 7.35 with its selected
  `LirBinOp.lhs` row.

Completion check: the external test no longer stops at the named pre-import
verifier failure and the handoff has no generic scalar authorization.
