# AMD64 `va_arg` Overflow Aggregate Carrier Authority Runbook

Status: Active
Source Idea: ideas/open/799_lir_amd64_vaarg_overflow_aggregate_carrier_authority.md
Supersedes: 753 Step 2 pending blocker resolution

## Purpose

Establish the one native authority boundary required for the AMD64 aggregate
`va_arg` overflow memcpy-like carrier, then hand its checked contract back to
753 without absorbing 753's broader memory/VA producer scope.

## Core Rule

Only structured overflow derivation, current-function ownership, source
object/storage, lifetime, and typed byte-size facts may authorize the carrier.
Never recover those facts from presentation text or accept an arbitrary derived
pointer as an overflow-area source.

## Read First

- `ideas/open/799_lir_amd64_vaarg_overflow_aggregate_carrier_authority.md`
- `ideas/open/753_lir_memory_va_pointer_authority_convergence.md` (resumption
  record)
- `src/codegen/lir/hir_to_lir/call/vaarg_amd64.cpp`
- Existing `LirVaArgOp`, pointer/object/lifetime verifier, and focused backend
  authority coverage adjacent to the AMD64 vaarg lowering seam

## Non-Goals

- Generic aggregate/vector authority, other targets, scalar `va_arg`, generic
  memory intrinsics, Raw-BIR, MIR, emission, or changing 753's source scope.

## Ordered Steps

### Step 1 - Define the checked overflow aggregate carrier contract

Goal: identify the exact native producer and verifier fields needed to prove
the source pointer derivation/base, object/storage, ownership, lifetime, and
typed size for the single AMD64 overflow aggregate memcpy row.

Actions:

- inspect the AMD64 overflow aggregate `va_arg` construction and current
  pointer/object/lifetime substrate;
- specify the accepted carrier relation and malformed boundaries without
  textual recovery;
- constrain unsupported aggregate forms to fail-closed or compatibility-only
  behavior.

Completion check: one bounded implementation packet can publish only the
identified structured facts and rejection rules.

### Step 2 - Publish and verify the native carrier boundary

Goal: construct the selected `LirMemcpyOp` carrier from checked structured
authority and reject invalid carrier facts before downstream use.

Actions:

- implement the minimum producer/verifier changes for the selected AMD64
  aggregate overflow route;
- add nearby accepted and malformed-carrier coverage;
- keep every nonmatching aggregate/vector and target route unchanged,
  fail-closed, or compatibility-only.

Completion check: a fresh build and focused AMD64 aggregate `va_arg` proof
show the accepted memcpy-like row and its native rejection boundary.

### Step 3 - Prove the carrier and hand it back to 753

Goal: record the exact checked facts and proof that permit 753 Step 2 to
select its matching producer packet.

Actions:

- run the fresh build and focused proof selected by the supervisor;
- document the carrier's fields, guarantees, rejected forms, and evidence in
  the blocker source at the smallest durable layer;
- return the handoff to 753 without modifying its scope or receiver work.

Completion check: the supervisor can resume 753 at its recorded Step 2 return
point using this one checked aggregate overflow carrier contract.
