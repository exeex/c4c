# AArch64 Materialized Pointer StoreLocal Writeback Refresh Runbook

Status: Active
Source Idea: ideas/open/361_aarch64_materialized_pointer_storelocal_writeback.md
Activated from: parked source idea with no active plan present

## Purpose

Refresh and, only if still current, resolve the AArch64 materialized
pointer-addressed `StoreLocal` writeback owner recorded in the source idea.

## Goal

Determine whether the current tree still has an AArch64 lowering failure where
stores through materialized pointer addresses fail to write back to the
addressed memory, then either close the idea or hand off a freshly localized
residual.

## Core Rule

Do not implement under this idea unless fresh generated-code evidence shows a
general materialized pointer-addressed store writeback failure.

## Read First

- `ideas/open/361_aarch64_materialized_pointer_storelocal_writeback.md`
- The parked outcome saying commit `ee027c36a` repaired materialized
  pointer-addressed `StoreLocal` writeback and split the then-current timeout
  residual to `ideas/open/362_aarch64_pointer_derived_load_address_scaling_timeout.md`.
- Current semantic BIR, prepared BIR, selected records, and generated AArch64
  artifacts for `c_testsuite_aarch64_backend_src_00181_c` only as
  representative evidence, not as a testcase-specific repair target.

## Current Scope

- AArch64 stores whose destination is a materialized pointer value.
- `StoreLocal` or equivalent selected memory operations that must write through
  a computed pointer instead of only updating a local value home.
- Focused backend coverage for explicit materialized pointer stores and the
  stack-homed pointer shape that historically represented the `00181` witness.
- Stability of the idea 360 starting-state repair and nearby `00170` / `00189`
  guardrails selected by the supervisor.

## Non-Goals

- Do not reopen pointer-derived load address scaling from idea 362 unless fresh
  evidence shows that the first bad fact has moved back into this store
  writeback owner and lifecycle routing confirms the boundary.
- Do not reopen direct `LoadGlobal` current-memory select-store handling from
  idea 360 except to keep it stable.
- Do not reopen stack-preserved pointer formal post-call overwrite, scalar
  formal post-call reloads, pointer-formal callee-saved home publication,
  semantic pointer-derived string loads, frontend admission, ABI composite
  work, variadic/floating work, dynamic-stack work, unrelated scalar
  compare/select residuals, expectations, unsupported classifications, runner
  behavior, timeout policy, CTest registration, or proof-log policy.
- Do not special-case `00181`, `Hanoi`, a tower name, one source line, one
  stack offset, one ABI register, or one emitted instruction neighborhood.

## Working Model

The historical failure was missing writeback for materialized
pointer-addressed stores: generated Hanoi moves could compute or reload a
pointer but fail to commit the store through that pointer, leaving later tower
state unchanged. The source idea says that owner was repaired and that the
next visible residual moved to pointer-derived load/address scaling under idea
362. This runbook therefore starts with a refresh and only proceeds to
implementation if the materialized store writeback owner is live again.

## Execution Rules

- Treat `ideas/open/361_aarch64_materialized_pointer_storelocal_writeback.md`
  as the durable scope contract.
- Prefer read-only generated artifact inspection before code edits.
- If focused materialized-store guardrails and the representative remain green
  or fail only outside this source scope, record that in `todo.md` and return
  for lifecycle close or deactivation.
- If the representative fails for an out-of-scope owner, classify the new
  first bad fact in `todo.md` rather than widening this plan.
- Any repair must preserve focused coverage for general materialized
  pointer-addressed store writeback and keep idea 360 starting-state behavior
  stable.
- For any code-changing step, require build proof plus the
  supervisor-selected focused CTest subset before acceptance.

## Step 1: Refresh Current First Bad Fact

Goal: determine whether materialized pointer-addressed `StoreLocal` writeback
is still a live first bad fact.

Primary target: the supervisor-selected focused representative for this idea,
normally `c_testsuite_aarch64_backend_src_00181_c` plus backend contracts for
materialized pointer stores, selected memory operands, and nearby `00170` /
`00189` stability.

Actions:

- Rebuild the current tree before classification.
- Run the focused representative and nearby backend coverage selected by the
  supervisor.
- If anything fails, inspect semantic BIR, prepared BIR, selected records, and
  generated AArch64 around the first materialized pointer-addressed store,
  expected target memory, emitted store behavior, and later consumer.
- Decide whether any observed failure still shows an in-scope materialized
  pointer-addressed store writeback fault.

Completion check:

- `todo.md` records either a current materialized pointer store writeback bad
  fact with concrete generated-code evidence, or a current out-of-scope /
  already-green classification for supervisor lifecycle routing.

## Step 2: Localize The Store Writeback Boundary

Goal: if Step 1 finds an in-scope failure, identify exactly where the
materialized pointer destination stops being preserved as an emitted store.

Primary target: prepared memory records, selected `StoreLocal` or equivalent
store nodes, materialized address carriers, and emitted AArch64 stores.

Actions:

- Compare semantic BIR, prepared BIR, selected records, and emitted assembly
  for the first pointer-addressed store that should mutate memory.
- Identify whether the owner is address-carrier publication, selected memory
  operand formation, store emission, local-value-home fallback, or another
  handoff with direct evidence.
- Record the smallest focused backend shape that should fail without the
  repair.

Completion check:

- The failing boundary is named in terms of source store operation,
  materialized address carrier, expected target memory, observed emitted
  behavior, and first affected consumer.

## Step 3: Repair General Materialized Store Writeback

Goal: repair the localized AArch64 store writeback rule without
testcase-shaped matching.

Actions:

- Implement the smallest shared change needed for materialized
  pointer-addressed stores to write through their addressed memory.
- Add or update focused backend coverage for a materialized pointer-addressed
  `StoreLocal` or equivalent store independent of `00181`.
- Preserve direct current-memory select-store handling, pointer-derived loads,
  recursive formal preservation, and nearby passing representatives.

Completion check:

- Focused coverage proves materialized pointer-addressed stores update the
  addressed memory through the repaired general rule.
- The implementation does not mention `00181`, `Hanoi`, one tower, one source
  line, one stack offset, one ABI register, or one emitted instruction
  neighborhood as a control path.

## Step 4: Prove And Reclassify

Goal: prove the repaired or refreshed owner and hand off any remaining first
bad fact.

Actions:

- Run the exact focused proof command delegated by the supervisor.
- Include `00181` representative proof if it remains the external witness.
- If the representative still fails, classify the new first bad fact and keep
  it separate from this materialized store writeback source scope.

Completion check:

- `todo.md` records the proof command and result.
- The source idea is either ready for close consideration, or the next first
  bad fact is clearly classified for lifecycle handoff.
