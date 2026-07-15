# LIR Call-Result Operand Carrier Foundation Runbook

Status: Active
Source Idea: ideas/open/785_lir_call_result_operand_carrier_foundation.md
Supersedes: 784 Step 2 while its generic call-result prerequisite is resolved.

## Purpose

Recover native current-function result authority at the direct generic call
factory boundary, then return the FP128 alignment carrier route to 784.

## Goal

Accept the smallest `make_lir_call_op` result carrier contract without
redesigning generic expression or call/argument families.

## Core Rule

Change only direct call-result transport. Do not make this factory boundary a
route into PHI semantics, generic call redesign, or target lowering.

## Read First

- `ideas/open/785_lir_call_result_operand_carrier_foundation.md`
- `ideas/open/784_lir_native_vaarg_operand_carrier_foundation.md`
- `src/codegen/lir/call_args_ops.hpp`
- direct `make_lir_call_op` declarations, implementations, and frontend-LIR
  structural-test inventory

## Non-Goals

- Do not modify PHI incoming transport/verification, predecessor/edge identity,
  Raw-BIR/importer, backend, target lowering, MIR, emission, vaarg helpers, or
  unrelated generic call/argument families.
- Do not recover result identity from text or compatibility spelling.

## Ordered Steps

### Step 1 - Inventory direct call-result carrier surfaces and probe feasibility

Goal: prove the smallest direct factory/support boundary that can retain a
native result for structured callers.

Actions:

- trace `make_lir_call_op` result construction, storage, and direct consumers;
- inspect whether the required support type can carry `LirOperand` without
  reaching unrelated generic call or argument families; and
- select focused frontend-LIR structural evidence for native factory-result
  retention.

Completion check:

- the direct carrier boundary and any narrower successor requirement are
  evidence-backed; no code or test change occurs.

### Step 2 - Bind and validate the minimal call-result carrier

Goal: retain a current-function native call result through the direct factory.

Actions:

- add only the selected direct result carrier and required compatibility
  plumbing;
- preserve the existing fail-closed validation boundary; and
- add focused structural plus applicable malformed-authority coverage.

Completion check:

- structured callers can observe the native factory result without text
  recovery or unrelated generic-family changes.

### Step 3 - Publish the 784 handoff

Goal: give 784 a verified, bounded prerequisite and exact retry point.

Actions:

- run the focused build/test proof and coordinate the supervisor-owned matching
  regression guard; and
- record the direct carrier contract and return 784 to Step 2, explicitly
  retaining its separate PHI incoming-value decision.

Completion check:

- 784 can retry its FP128 ptrmask carrier route without reopening 785; any
  non-direct support requirement is named as a successor.
