# LIR Native Vaarg Operand Carrier Foundation Runbook

Status: Active
Source Idea: ideas/open/784_lir_native_vaarg_operand_carrier_foundation.md
Resumed from: closed `ideas/closed/785_lir_call_result_operand_carrier_foundation.md`

## Purpose

Finish the minimal native vaarg operand/result carrier foundation now that the
FP128 ptrmask direct call result retains its native authority.

## Goal

Establish the smallest carrier contract covering AArch64 GP, AArch64
FP/alignment, and AMD64 register/stack seams, then make the distinct
value-only-PHI-transport decision required to return probe binding to 783.

## Core Rule

Use only native current-function authority. 785 solved the direct FP128 call
result boundary; it does not authorize PHI verification, predecessor/edge work,
or broad generic-expression changes.

## Completed Work

- Step 1, `Inventory carrier surfaces and prove structural-probe feasibility`,
  accepted at commit `b96751b03`.
- Closed 785 provides commit `3b53451c0`: the FP128 ptrmask direct call result
  now reaches its immediate typed GEP consumer as native authority.

## Read First

- `ideas/open/784_lir_native_vaarg_operand_carrier_foundation.md`
- `ideas/closed/785_lir_call_result_operand_carrier_foundation.md`
- `ideas/open/783_lir_native_vaarg_operand_result_seam_decomposition.md`
- `ideas/open/782_lir_vaarg_phi_input_result_identity_publication.md`
- `ideas/open/751_lir_phi_incoming_value_and_predecessor_identity.md`
- `src/codegen/lir/hir_to_lir/call/vaarg.cpp`
- `src/codegen/lir/hir_to_lir/call/vaarg_amd64.cpp`

## Non-Goals

- Do not modify PHI verification, predecessor/edge identity, Raw-BIR/importer,
  backend, target lowering, MIR, emission, broad generic-expression redesign,
  or 782 helper fields.
- Do not recover authority from names, labels, text, output, instruction order,
  side tables, or result-name maps.

## Ordered Steps

### Step 1 - Inventory carrier surfaces and prove structural-probe feasibility (complete)

Completion: accepted at `b96751b03`. The shared carrier route and the
remaining PHI incoming-value question were recorded before implementation.

### Step 2 - Bind the minimal generic carrier contract

Goal: publish only the native current-function operand/result carrier required
by all three seam boundaries.

Actions:

- retry the previously interrupted contract with 785's direct FP128 ptrmask
  result carrier available;
- bind the smallest remaining producer/consumer carrier additions across GP,
  FP/alignment, and AMD64 register/stack; and
- decide whether value-only PHI incoming transport is indispensable, adding it
  only as transport of existing native authority and excluding verification and
  CFG semantics.

Completion check:

- all three seams retain native structural authority at their immediate
  consumer boundaries with no text-recovery fallback or PHI-completion claim.

### Step 3 - Prove the carrier and publish the 783 handoff

Goal: establish focused structural evidence and an unambiguous parent return.

Actions:

- add focused frontend-LIR structural coverage for GP, FP/alignment, and AMD64
  register/stack carrier propagation plus relevant malformed authority cases;
- run the focused build/test command and coordinate the supervisor-owned
  matching regression guard; and
- record the accepted contract, PHI incoming-value decision, and exact return
  point to 783 Step 3.

Completion check:

- 783 can resume its three focused probe bindings without reopening this
  foundation; any non-transport PHI requirement is named as a separate
  successor.
