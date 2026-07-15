# LIR Aggregate and Vector Value Identity Convergence Runbook

Status: Active
Source Idea: ideas/open/754_lir_aggregate_vector_value_identity_convergence.md
Resumed from: closed 814 poison-second-shape carrier blocker at Step 9

## Purpose

Complete the remaining representative aggregate/vector rows one bounded,
row-local authority route at a time, preserving the accepted ExtractValue and
InsertValue work.

## Core Rule

Use checked current-function structured IDs and row-specific typed facts. Do
not recover result, operand, index, mask, or type identity from display text,
instruction order, rendered LLVM, or testcase names. Keep unselected rows
unchanged and fail closed.

## Read First

- `ideas/open/754_lir_aggregate_vector_value_identity_convergence.md`
- `ideas/closed/811_lir_native_vector_authority_carrier_publication.md`
- `ideas/closed/814_lir_shuffle_vector_poison_second_shape_carrier_repair.md`
- `src/codegen/lir/ir.hpp` and `src/codegen/lir/verify.cpp`

## Non-Goals

- Repeat accepted Steps 1--8, or 811/814 prerequisite publication work.
- Absorb CFG/PHI, generic provenance/layout, Raw-BIR, target/MIR/emission, or
  display-text recovery.
- Treat the prerequisite full baseline as proof of a 754 vector row.

## Accepted Steps

### Step 1 - Audit and select one aggregate/vector authority row — complete

Accepted in `d8e5ed3a8`: selected only `LirExtractValueOp`.

### Step 2 - Repair structured result and aggregate operand authority — complete

Accepted in `33a6c21cc` with focused 6/6 proof and a supervisor-accepted
3037/3037 full baseline. Do not reopen its selected ExtractValue authority.

### Step 3 - Verify row-specific index facts — complete

Accepted in `97137f39d`: the selected ExtractValue route rejects invalid
field indices and incoherent result element types using 801's native layout.

### Step 4 - Prove and hand off the bounded row — complete

Accepted with fresh full CTest before/after at 3037/3037 and the matching
guard. It closes only the selected ExtractValue row.

### Step 5 - Audit and select the next remaining representative row — complete

Accepted in `270c6a93e`: selected only terminal direct-complex
`LirInsertValueOp`.

### Step 6 - Implement and prove the selected remaining row — complete

Accepted in `8fe6c3569` with nearby valid/malformed coverage, matching
`^backend_` 5/5 guard, and accepted 3037/3037 full baseline.

### Step 7 - Reassess remaining source completion — complete

Closure correctly rejected: InsertElement, ExtractElement, and ShuffleVector
remain. The next route was a bounded vector audit only.

### Step 8 - Audit and select one remaining vector authority row — complete

The audit was accepted in `1636ebe90`; closed 811 later supplied the native
vector carrier. The former Step 9 InsertElement attempt was rejected before
implementation because unselected ShuffleVector carrier prerequisites broke
the full baseline. Do not reuse that selection.

## Remaining Steps

### Step 9 - Implement and prove the Step 8 selection

Goal: before implementation, make a fresh one-row vector audit/selection
decision against the accepted 811/814 carrier handoffs, then implement only
that selected row if it has a complete row-local contract.

Actions:

- inspect only `LirInsertElementOp`, `LirExtractElementOp`, and
  `LirShuffleVectorOp` seams; select exactly one only after recording the
  concrete seam, positive/malformed matrix, and two excluded rows in `todo.md`;
- do not reuse the rejected InsertElement selection; do not infer authority
  from rendered text or treat 814's repair as a ShuffleVector row claim;
- if no complete row-local contract exists, preserve this exact Step 9 return
  point and route a separately scoped prerequisite rather than widening 754;
- after a selection, publish and verify only its structured result/use
  authority and typed facts; prove it with fresh build, same-feature test,
  matching guard, and any supervisor-selected full checkpoint.

Completion check: one freshly selected row alone is structurally authoritative
and proven, or an exact out-of-scope blocker route is recorded. The two
unselected rows remain unchanged.

### Step 10 - Reassess remaining source completion

Goal: make the explicit lifecycle decision after the bounded Step 9 packet.

Completion check: repair to a one-row audit, switch to an atomic blocker, or
source-closure evidence; never silently widen Step 9.
