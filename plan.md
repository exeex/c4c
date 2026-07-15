# LIR Aggregate SSA Producer Authority Publication Runbook

Status: Active
Source Idea: ideas/open/803_lir_aggregate_ssa_producer_authority_publication.md
Activated from: 754 Step 2 aggregate-use authority repair. Return to 754 only
after this bounded producer/operand handoff is accepted.

## Purpose

Preserve native aggregate SSA producer authority for the two selected non-call
paths required by 754's extractvalue use verification.

## Core Rule

Structured aggregate SSA values require checked current-function native
authority. Do not recover it from text or evade it through a raw fallback.

## Read First

- `ideas/open/803_lir_aggregate_ssa_producer_authority_publication.md`
- `ideas/open/754_lir_aggregate_vector_value_identity_convergence.md` resumption record
- `ideas/closed/798_lir_operand_provenance_authority_publication.md`
- local-load, `LirInsertValueOp`, and `LirExtractValueOp` lowering/verifier seams

## Non-Goals

- Generic operand or expression API conversion, broad producer-family work,
  extractvalue row/index/layout/result-type validation, Raw-BIR, or text
  recovery.

## Ordered Steps

### Step 1 - Trace and select aggregate producer authority seams

Goal: identify the minimal existing authority path for the failing aggregate
local-load and constructed-insertvalue values, from producer creation through
the extractvalue consumer/verifier boundary.

Actions:

- reproduce and trace `positive_sema_ok_call_builtin_runtime_c` and the
  direct-conjugate aggregate path;
- identify which existing `LirValueId` is lost or rejected for each selected
  producer and define the smallest shared-or-separate native carrier;
- state exact type/ownership/display-mirror checks and malformed proof seams;
- leave all unselected producer kinds and extractvalue row facts untouched.

Completion check: an implementation-ready contract identifies both selected
producer paths, their valid authority boundary, and rejection cases without
text recovery or raw fallback.

### Step 2 - Publish checked authority for selected aggregate producers

Goal: carry the selected aggregate producer IDs natively to downstream uses
and validate ownership/type coherence.

Actions:

- first restore the rejected 803-specific implementation hunks before the next
  attempt; do not build further work on them and do not disturb preserved
  unaccepted 801/802 changes in shared files;
- gate the generalized producer verification on an explicit selected
  `extract.requires_native_result_authority` contract, so legacy SSA extracts
  such as `backend_lir_to_bir_interface` retain their existing route;
- publish native result IDs only for the selected aggregate local loads and
  terminal constructed insertvalues, preserving their operands through the
  immediate unary extract path;
- add a dedicated aggregate producer-type carrier for those opt-in producers
  and compare it with `extract.agg_type`; leave the local-object raw pointee
  and `LirAllocaOp.local_object_authority` equality contract unchanged;
- verify only selected extracts against current-function producer ID, allowed
  opted-in Call/Load/Insert kind, aggregate carrier/type equality, and exact
  display mirror. All other kinds fail closed for selected extracts.

Completion check: selected local-load and constructed-insertvalue aggregate
uses retain checked current-function authority; legacy extracts remain
ungated, local-object raw-pointee authority is unchanged, and unrelated
producer families fail closed.

### Step 3 - Prove and publish the 754 handoff

Goal: establish positive and malformed proof for the two selected paths and
record the exact downstream contract.

Completion check: fresh build, focused aggregate/frontend/backend proof, and
supervisor-selected broader acceptance support resuming 754 Step 2; do not
claim extractvalue index/layout/result validation.
