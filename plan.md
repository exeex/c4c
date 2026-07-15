# LIR Native Vaarg Operand/Result Seam Decomposition Runbook

Status: Active
Source Idea: ideas/open/783_lir_native_vaarg_operand_result_seam_decomposition.md
Resumed after: closed `ideas/closed/784_lir_native_vaarg_operand_carrier_foundation.md`

## Purpose

Turn the accepted native carrier foundation into the three focused structural
probe bindings required before 782 helper-field work can resume.

## Goal

Accept the narrowest viable native vaarg operand/result contract for AArch64
GP, AArch64 FP/alignment, and AMD64 reg/stack chains, then return to 782.

## Core Rule

783 owns focused frontend-LIR decomposition evidence only. The accepted 784
change makes `LirPhiIncoming.value` native `LirOperand` transport; it does not
authorize PHI verification, predecessor/edge identity, CFG semantics, or a
claim of PHI completion. Labels remain string-only.

## Completed Work

- Step 1, `Establish the focused frontend-LIR baseline`, accepted at
  `0d0d402d6`.
- Step 2, `Enumerate the three native vaarg structural seams`, accepted at
  `fb1cb983b`.
- 784 prerequisite accepted at `e45a6b0ee`: native carrier authority now
  reaches the immediate consumers for the GP, FP/alignment, and AMD64 seams.

## Read First

- `ideas/open/783_lir_native_vaarg_operand_result_seam_decomposition.md`
- `ideas/closed/784_lir_native_vaarg_operand_carrier_foundation.md`
- `ideas/open/782_lir_vaarg_phi_input_result_identity_publication.md`
- `ideas/open/751_lir_phi_incoming_value_and_predecessor_identity.md`
- `src/codegen/lir/hir_to_lir/call/vaarg.cpp`
- `src/codegen/lir/hir_to_lir/call/vaarg_amd64.cpp`
- `tests/frontend/frontend_lir_call_type_ref_test.cpp`

## Non-Goals

- Do not modify PHI verification, predecessor/edge authority, Raw-BIR/importer,
  backend, target lowering, MIR, or emission.
- Do not begin 782 helper-field publication, generic migration, or broad API
  redesign.
- Do not use names, labels, rendered output, instruction order, testcase text,
  side tables, or result-name maps as identity authority.
- Do not extend the accepted value-only `LirPhiIncoming` transport into PHI
  completion.

## Execution Rules

1. Use frontend-LIR structural probes, not `tests/backend/case/`.
2. Give each probe one primary contract, bound to exactly one named chain.
3. Preserve 784's accepted transport boundary; do not re-litigate it or expand
   it into verification/CFG work.
4. Build and run the focused test after code/test changes. Coordinate matching
   regression evidence with the supervisor before accepting the route.

## Ordered Steps

### Step 1 - Establish the focused frontend-LIR baseline (complete)

Completion: accepted at `0d0d402d6`; the focused command and frontend-LIR
probe location were recorded before implementation.

### Step 2 - Enumerate the three native vaarg structural seams (complete)

Completion: accepted at `fb1cb983b`; the AArch64 GP, AArch64 FP/alignment, and
AMD64 register/stack source, propagation, consumer boundary, and probe targets
were recorded.

### Step 3 - Bind focused probes to native operand/result contracts

Goal: turn the completed inventory and 784 carrier foundation into a complete,
fail-closed three-chain decomposition contract set.

Actions:

- add or refine one focused frontend-LIR structural probe for each of the
  AArch64 GP `gr_top` to `reg_addr`, AArch64 FP/alignment, and AMD64
  register/stack seams;
- specify the authoritative source, required native propagation, and immediate
  consumer boundary for every probe; and
- prove the probes distinguish missing/raw authority from valid structural
  propagation without PHI verification or predecessor/edge work.

Completion check:

- all three probes are bound to one narrow native operand/result contract and
  the focused evidence is sufficient to choose the smallest implementation
  seam.

### Step 4 - Accept the narrowest contract and return to 782

Goal: conclude the decomposition route with an executable parent return point.

Actions:

- compare the three bound contracts and select only the common minimal native
  operand/result publication required by 782;
- record accepted proof and implementation commit references; and
- request lifecycle resumption of 782 Step 1, preserving its source record.

Completion check:

- 782 can resume its Step 1 helper-input publication with all three contracts
  named; no generic-migration or PHI-completion claim is made.
