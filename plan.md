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

### Step 9 - Implement and prove the Step 8 selection — complete

Accepted in `88268afe6`: freshly selected only the scalar-to-vector
zero-initializer splat `LirShuffleVectorOp` route. It requires a native
carrier, the immediately preceding native InsertElement result as first vector
use, structured poison-second shape, equal vector shapes, and selected-zero
mask lanes. Nearby positive/malformed coverage and the matching backend guard
passed 6/6; representative `scal-to-vec1` emission and the supervisor-owned
fresh 3038/3038 full CTest baseline were accepted. InsertElement and
ExtractElement remain unselected and unchanged.

### Step 10 - Reassess remaining source completion — complete

Closure is rejected: the source still requires representative InsertElement
and ExtractElement structured-authority rows and vector insert/extract/shuffle
chain coverage. Continue through a fresh one-row audit without widening the
accepted shuffle route.

## Remaining Steps

### Step 11 - Audit and select one remaining vector authority row

Goal: inspect only `LirInsertElementOp` and `LirExtractElementOp` against the
accepted 811 carrier. Select exactly one only after recording its concrete
seam, complete positive/malformed matrix, and excluded row in `todo.md`.

Completion check: one complete row-local contract is selected, or an exact
separate-blocker route preserves this Step 11 return point. Do not infer facts
from display text or reuse the accepted shuffle proof as an insert/extract
claim.

### Step 12 - Implement and prove the Step 11 selection

Goal: publish and verify only the selected row's structured result/use
authority and typed index/element facts, with fresh build, nearby same-feature
coverage, matching backend guard, and a supervisor-selected full checkpoint.

Completion check: the selected row is structurally authoritative and proven;
the unselected row remains unchanged. Then return for an explicit source
completion decision.
