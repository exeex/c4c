# AArch64 Named Handoff Materializer Cleanup Runbook

Status: Active
Source Idea: ideas/open/709_aarch64_named_handoff_materializer_cleanup.md
Activated from: handback after closure of idea 729

## Purpose

Finish retiring executable route/index reconstruction from AArch64 MIR
materialization after the common current-block query boundary was repaired.

## Goal

Remove the remaining locally replaceable address-materialization lookup and
prove that semantic AArch64 materialization consumes common named/prepared
authority without target-local route or index recreation.

## Core Rule

Consume existing attached common authority and keep only target instruction
and ABI realization. Do not recreate semantic lookups under new names.

## Read First

- `ideas/open/709_aarch64_named_handoff_materializer_cleanup.md`
- `ideas/closed/729_common_current_block_join_query_exposure.md`
- the AArch64 address materializer and directly implicated common query APIs

## Current Scope

- The locally replaceable address-materialization lookup reconstruction found
  by the prior Step 3 audit.
- The final semantic AArch64 route/index retirement search.
- Focused address-materialization proof and a supervisor-selected broader
  AArch64/backend checkpoint.

## Non-Goals

- No common producer or query-contract redesign.
- No reopening the completed current-block or return-chain authority work.
- No x86 or RV64 migration, target policy change, expectation weakening, or
  testcase-shaped fallback.
- Debug-only route vocabulary is not semantic authority and belongs to idea
  712 when it cannot be removed locally and safely.

## Execution Rules

- Replace only executable reconstruction that an existing attached common
  query already owns.
- Validate owner and stable identity and fail closed when required authority is
  absent or inconsistent.
- Stop for lifecycle review if the remaining address relation is not expressible
  through an existing common contract.
- Delete obsolete builders/indexes after their final consumer disappears; do
  not rename or wrap them.

## Ordered Steps

### Step 3.1: Remove the address-materialization reconstruction

Goal: replace the remaining locally rebuilt address lookup with existing
attached common authority.

Primary target: the AArch64 address materializer identified by the retirement
audit and only its directly implicated callers.

Actions:

- Confirm the remaining lookup is executable semantic state rather than
  target-local instruction realization.
- Consume the existing typed attached query with owner and identity checks.
- Delete the local builder, scan, or fallback after its final consumer is gone.
- Preserve relocation, addressing, and ABI behavior and fail closed on missing
  or inconsistent authority.
- Run the supervisor-selected build and focused address/materialization tests
  without expectation changes.

Completion check:

- Address materialization no longer rebuilds semantic lookup state, focused
  proof is green, and no common contract or testcase expectation changed.

### Step 3.2: Prove retirement and disposition

Goal: demonstrate that semantic AArch64 materialization has no remaining route
record, route index, or recreated lookup authority.

Actions:

- Search the scoped AArch64 semantic owner for executable route/index use and
  renamed forms of the retired current-block and address reconstructions.
- Classify surviving route-labelled text as non-semantic debug vocabulary for
  idea 712, or remove it when local and safe.
- Run the supervisor-selected broader AArch64/backend regression comparison.
- Review the complete idea 709 implementation against its reject signals.

Completion check:

- The semantic retirement guard is zero, broader proof has no new failures,
  and no route/index recreation or expectation weakening remains.
