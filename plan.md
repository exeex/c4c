# AArch64 Named Handoff Materializer Cleanup Runbook

Status: Active
Source Idea: ideas/open/709_aarch64_named_handoff_materializer_cleanup.md

## Purpose

Remove executable route-record authority from AArch64 MIR materialization while
preserving target-local instruction and ABI realization.

## Goal

Make AArch64 materializers consume the common named and prepared handoff views,
fail closed when required authority is unavailable, and retire semantic route
and route-index dependencies from the target codegen owner.

## Core Rule

Migrate consumers to existing common authority. Do not recreate route analysis,
route indexes, or target-local semantic fallbacks under new names.

## Read First

- `ideas/open/709_aarch64_named_handoff_materializer_cleanup.md`
- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- the common named/prepared query interfaces established by idea 706

## Current Scope

- AArch64 dispatch, calls, globals, ALU, comparison, select, publication, and
  value materialization.
- Existing AArch64 instruction-dispatch, call-boundary, branch-control,
  current-block/join, scalar-ALU, and memory-operand proof surfaces.
- Removal of semantic route records and target-local route indexes from the
  AArch64 materialization owner.

## Non-Goals

- No common producer or query-contract redesign.
- No x86 or RV64 consumer migration.
- No target instruction or ABI policy changes.
- No expectation weakening, testcase-shaped fallback, or assembly-only proof.
- Debug vocabulary may remain only when it cannot affect lowering.

## Working Model

- Common named/prepared views own semantic handoff authority.
- AArch64 owns only legal instruction selection and ABI realization.
- Missing or inconsistent prepared authority must fail closed.
- Route-labelled state must not select executable behavior.

## Execution Rules

- Start at `src/backend/mir/aarch64/codegen/dispatch.cpp` and follow only its
  directly implicated AArch64 sibling materializers.
- Replace consumers with existing typed/common queries; do not copy producer
  reasoning into target helpers.
- Keep each packet behavior-preserving and prove more than one narrow fixture
  before retiring a fallback.
- Stop for lifecycle review if an existing common query cannot express required
  authority without a producer-contract change.

## Ordered Steps

### Step 1: Inventory and migrate AArch64 dispatch authority

Goal: establish the bounded first consumer migration without duplicating route
semantics.

Primary target: `src/backend/mir/aarch64/codegen/dispatch.cpp`

Actions:

- Inventory route records, route indexes, and fallback decisions used by
  dispatch and identify their existing common named/prepared replacements.
- Migrate one coherent dispatch family to those replacements.
- Make missing or inconsistent required authority fail closed.
- Add or update focused proof for instruction dispatch and the implicated
  control or value family without weakening expectations.

Completion check:

- The migrated dispatch family has no executable route dependency, retains
  target-local behavior, and passes the supervisor-selected build plus focused
  AArch64 proof.

### Step 2: Migrate sibling AArch64 materializers

Goal: remove semantic route ownership across the remaining AArch64 consumer
families.

Actions:

- Migrate calls, globals, ALU, comparison, select, publication, and value
  materialization in reviewable packets.
- Delete target-local route-index construction and semantic fallbacks as their
  final consumers disappear.
- Preserve target instruction and ABI policy.
- Prove call-boundary, branch-control, current-block/join, scalar-ALU, and
  memory-operand behavior across the affected packets.

Completion check:

- Every scoped materializer consumes common named/prepared views, missing
  authority fails closed, and nearby same-feature coverage remains green.

### Step 3: Prove retirement and disposition

Goal: demonstrate that semantic AArch64 materialization no longer depends on
route vocabulary or recreated indexes.

Actions:

- Search the scoped semantic owner for remaining executable route/index use.
- Classify any surviving route-labelled text as non-semantic debug vocabulary
  owned by idea 712, or remove it when local and safe.
- Run the supervisor-selected broader AArch64 validation checkpoint.
- Review the complete slice against the source idea reject signals.

Completion check:

- The retirement guard is zero for semantic AArch64 materialization, broad
  behavior proof is green, and no route/index recreation or expectation
  weakening remains.
