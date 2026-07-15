# LIR Memory/VA Pointer Authority Convergence Runbook

Status: Active
Source Idea: ideas/open/753_lir_memory_va_pointer_authority_convergence.md
Resumed from: 753 Step 2 after closed 799 overflow aggregate carrier handoff;
Step 2 is satisfied by accepted 799 evidence with no new 753 semantic delta

## Purpose

Finish the bounded native memory/VA producer proof and publish one
receiver-ready handoff. The one AMD64 aggregate `va_arg` overflow memcpy row
is already selected, published, and verified by closed 799; do not duplicate
that completed semantic work or widen the producer family.

## Core Rule

Native structured current-function authority is the sole semantic input. For
the resumed row, consume 799's checked carrier exactly as published; do not
recover pointer, object, lifetime, size, or row-selection facts from text or
redefine the aggregate/vector carrier boundary.

## Read First

- `ideas/open/753_lir_memory_va_pointer_authority_convergence.md` (resumed
  carrier handoff record)
- `ideas/closed/799_lir_amd64_vaarg_overflow_aggregate_carrier_authority.md`
- `src/codegen/lir/hir_to_lir/call/vaarg_amd64.cpp`
- Existing `LirVaArgOp`, pointer/object/lifetime verifier, and focused backend
  authority coverage adjacent to the AMD64 vaarg lowering seam

## Non-Goals

- Aggregate/vector carrier publication or generalization, other targets,
  scalar `va_arg`, generic memory intrinsics, Raw-BIR, MIR, emission, or
  changing 753's source scope.

## Ordered Steps

### Step 1 - Establish the bounded native memory/VA authority boundary (complete)

Completed accepted 753 work includes direct-local `va_start`/`va_end`,
positive-size aggregate `memset`, direct-local `va_copy`, and AMD64
scalar/pointer `va_arg`. Their retained evidence is recorded in the source
handoff record.

Completion check: complete; do not redo these accepted packets.

### Step 2 - Consume and verify the checked aggregate overflow carrier (complete; no delta)

Goal: establish whether the selected 753 row needs any consumer work beyond
closed 799's carrier contract, while retaining 799 as the sole publisher of
derived aggregate carrier facts.

Actions:

- inspected the closed 799 carrier contract and the resumed AMD64 aggregate
  overflow producer seam;
- accepted `c4e820a48` as the complete matching selection/publication and
  verification: `vaarg_amd64.cpp` selects the row and publishes
  `requires_native_memory_va_authority` plus its carrier; `verify.cpp`
  consumes/verifies it; nearby focused coverage proves positive and malformed
  boundaries;
- recorded no new 753 semantic delta. All nonmatching aggregate/vector and
  target routes remain fail-closed or compatibility-only.

Completion check: satisfied by `c4e820a48`: fresh build plus
`./build/tests/backend/bir/backend_lir_selected_pointer_authority_test` passed,
the matching `test_after.log` baseline is 5/5, and the supervisor comparison
is non-decreasing 5/5. Do not redo this as a new code claim.

### Step 3 - Prove the bounded producer slice and hand off one receiver row

Goal: meet 753's source proof gate and publish exactly one later receiver
handoff without Raw-BIR receipt work.

Actions:

- run a fresh build and focused memory/VA producer/verifier proof;
- run the source-required full baseline; closure requires 100% passing tests,
  and a lower baseline must be diagnosed through `log/*` by time/commit before
  continuing;
- document exactly one selected receiver handoff with native fields,
  guarantees, rejected forms, and accepted proof.

Completion check: 753 has source-complete producer authority evidence and one
receiver-ready handoff; receiver work remains outside this runbook.
