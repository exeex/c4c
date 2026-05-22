# AArch64 Pointer-Derived Load Address Scaling Refresh Runbook

Status: Active
Source Idea: ideas/open/362_aarch64_pointer_derived_load_address_scaling_timeout.md
Activated from: parked source idea with no active plan present

## Purpose

Refresh and, only if still current, resolve the AArch64 pointer-derived load
address-scaling owner recorded in the source idea.

## Goal

Determine whether the current tree still has an AArch64 lowering failure where
pointer-derived loads or address scaling can produce the post-writeback
`00181` timeout, then either close the idea or hand off a freshly localized
residual.

## Core Rule

Do not implement under this idea unless fresh generated-code evidence shows a
general pointer-derived load or address-scaling failure.

## Read First

- `ideas/open/362_aarch64_pointer_derived_load_address_scaling_timeout.md`
- The parked lifecycle note saying commit `321031ce0` repaired the
  pointer-derived load/address-scaling timeout owner and split the next
  observed stale-select-join residual to
  `ideas/open/363_aarch64_prepared_select_condition_join_stale_reload.md`.
- Current semantic BIR, prepared BIR, selected records, and generated AArch64
  artifacts for `c_testsuite_aarch64_backend_src_00181_c` only as
  representative evidence, not as a testcase-specific repair target.

## Current Scope

- AArch64 pointer-derived loads whose address is computed from a pointer value.
- Address scaling for pointer/index carriers feeding load operations.
- Loop or recursive consumers that can timeout or misbehave when the emitted
  load address calculation is wrong.
- Stability of the idea 360 starting-state repair, idea 361 materialized
  pointer store writeback, and nearby `00170` / `00189` guardrails selected by
  the supervisor.

## Non-Goals

- Do not reopen materialized pointer-addressed store writeback from idea 361
  except to keep it stable.
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

The historical failure was a post-store-writeback timeout in `00181` with
suspicious pointer-derived load/address-scaling code such as immediate-scale
materialization clobbering the live index or result carrier. The source idea
says that owner was repaired and the next visible residual moved to stale
prepared select/join reload behavior. This runbook therefore starts with a
refresh and only proceeds to implementation if the pointer-derived
load/address-scaling owner is live again.

## Execution Rules

- Treat `ideas/open/362_aarch64_pointer_derived_load_address_scaling_timeout.md`
  as the durable scope contract.
- Prefer read-only generated artifact inspection before code edits.
- If focused pointer-load guardrails and the representative remain green or
  fail only outside this source scope, record that in `todo.md` and return for
  lifecycle close or deactivation.
- If the representative fails for an out-of-scope owner, classify the new
  first bad fact in `todo.md` rather than widening this plan.
- Any repair must preserve focused coverage for general pointer-derived
  load/address scaling and keep idea 360 and idea 361 behavior stable.
- For any code-changing step, require build proof plus the
  supervisor-selected focused CTest subset before acceptance.

## Step 1: Refresh Current First Bad Fact

Goal: determine whether pointer-derived load/address scaling is still a live
first bad fact.

Primary target: the supervisor-selected focused representative for this idea,
normally `c_testsuite_aarch64_backend_src_00181_c` plus backend contracts for
pointer-derived loads, selected memory operands, materialized pointer stores,
and nearby `00170` / `00189` stability.

Actions:

- Rebuild the current tree before classification.
- Run the focused representative and nearby backend coverage selected by the
  supervisor.
- If anything fails, inspect semantic BIR, prepared BIR, selected records, and
  generated AArch64 around the first pointer-derived load, scale/index
  calculation, emitted load address, and timeout-causing consumer.
- Decide whether any observed failure still shows an in-scope
  pointer-derived load/address-scaling fault.

Completion check:

- `todo.md` records either a current pointer-derived load/address-scaling bad
  fact with concrete generated-code evidence, or a current out-of-scope /
  already-green classification for supervisor lifecycle routing.

## Step 2: Localize The Load Address Boundary

Goal: if Step 1 finds an in-scope failure, identify exactly where the
pointer-derived load address or scale calculation stops preserving the
intended pointer/index carrier.

Primary target: semantic memory operations, prepared memory records, selected
load/address nodes, scale materialization, and emitted AArch64 loads.

Actions:

- Compare semantic BIR, prepared BIR, selected records, and emitted assembly
  for the first pointer-derived load that should read through a computed
  address.
- Identify whether the owner is pointer-carrier publication, scale/index
  materialization, selected memory operand formation, load emission, or another
  handoff with direct evidence.
- Record the smallest focused backend shape that should fail without the
  repair.

Completion check:

- The failing boundary is named in terms of source load operation,
  pointer/index carrier, expected address, observed emitted address
  calculation, and first affected consumer.

## Step 3: Repair General Pointer-Derived Load Scaling

Goal: repair the localized AArch64 load/address-scaling rule without
testcase-shaped matching.

Actions:

- Implement the smallest shared change needed for pointer-derived loads to
  preserve the correct pointer/index carrier and address scale.
- Add or update focused backend coverage for the localized pointer-derived
  load/address-scaling shape independent of `00181`.
- Preserve materialized pointer store writeback, direct current-memory
  select-store handling, recursive formal preservation, and nearby passing
  representatives.

Completion check:

- Focused coverage proves pointer-derived loads compute the intended address
  through the repaired general rule.
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
  it separate from this pointer-derived load/address-scaling source scope.

Completion check:

- `todo.md` records the proof command and result.
- The source idea is either ready for close consideration, or the next first
  bad fact is clearly classified for lifecycle handoff.
