# LIR Memory/VA Pointer Authority Convergence Runbook

Status: Active
Source Idea: ideas/open/753_lir_memory_va_pointer_authority_convergence.md
Resumed from: resolved 800 unselected-alloca compatibility blocker at 753 Step 3

## Purpose

Finish the bounded native memory/VA producer proof by documenting exactly one
receiver-ready handoff. The selected AMD64 aggregate `va_arg` overflow carrier
is already published and verified; do not duplicate its semantic work.

## Core Rule

Native structured current-function authority is the sole semantic input. Do
not recover pointer, object, lifetime, size, or row-selection facts from text,
or redefine the aggregate/vector carrier boundary.

## Read First

- `ideas/open/753_lir_memory_va_pointer_authority_convergence.md`
- `ideas/closed/800_lir_amd64_vaarg_unselected_alloca_compatibility_regression.md`
- `ideas/closed/799_lir_amd64_vaarg_overflow_aggregate_carrier_authority.md`
- `src/codegen/lir/hir_to_lir/call/vaarg_amd64.cpp`
- Existing `LirVaArgOp`, pointer/object/lifetime verifier, and focused backend
  authority coverage adjacent to the AMD64 vaarg lowering seam

## Non-Goals

- Aggregate/vector carrier publication or generalization, other targets,
scalar `va_arg`, generic memory intrinsics, Raw-BIR receipt/lowering, MIR,
emission, or any selected-carrier authority change.

## Ordered Steps

### Step 1 - Establish the bounded native memory/VA authority boundary (complete)

Completed accepted 753 work includes direct-local `va_start`/`va_end`,
positive-size aggregate `memset`, direct-local `va_copy`, and AMD64
scalar/pointer `va_arg`. Their retained evidence is recorded in the source
handoff record.

Completion check: complete; do not redo these accepted packets.

### Step 2 - Consume and verify the checked aggregate overflow carrier (complete; no delta)

Accepted closed-799 commit `c4e820a48` already selects the one AMD64
aggregate-overflow row, publishes its checked carrier, and verifies it. Its
fresh build plus focused backend proof and non-decreasing 5/5 comparison are
recorded in the source idea.

Completion check: complete with no new 753 semantic delta; do not republish
or generalize the carrier.

### Step 3 - Document exactly one receiver-ready handoff with native fields, guarantees, rejected forms, and accepted proof

Goal: record the one source-required receiver handoff without Raw-BIR receipt
work.

Actions:

- document exactly one selected receiver handoff with its native fields,
  guarantees, rejected forms, and accepted proof;
- retain the accepted fresh build, focused frontend proof, and matching
  3037/3037 full-baseline guard comparison from resolved 800 as the source
  proof gate;
- do not implement Raw-BIR receipt/lowering or redo accepted Steps 1/2.

Completion check: the source contains exactly one receiver-ready handoff with
native facts and rejection boundary; receiver implementation remains outside
this runbook.
