# RV64 Scalar Call-Boundary Freshness After Call Runbook

Status: Active
Source Idea: ideas/open/643_rv64_scalar_call_boundary_freshness_after_call.md

## Purpose

Repair the RV64 scalar call-boundary freshness policy exposed by
`src/ipa-sra-2.c`, where a scalar value needed after a call is consumed from an
ABI-clobbered register instead of an explicit fresh source.

## Goal

Move one complete-authority scalar post-call value path past stale
clobbered-register consumption, or record the exact missing preservation,
republication, or rematerialization authority.

## Core Rule

Do not treat a pre-call register home as fresh after a call unless explicit
preservation, republication, or rematerialization authority proves that scalar
value is still valid.

## Read First

- ideas/open/643_rv64_scalar_call_boundary_freshness_after_call.md
- docs/target_abi_contract_research/05_prior_preservation_freshness_and_stale_home_risk.md
- src/ipa-sra-2.c residual logs under `build/rv64_gcc_c_torture_backend/`

## Current Scope

- Refresh the `src/ipa-sra-2.c` RV64 residual after the large selected
  pointer-offset access fixed by idea 634.
- Identify the prepared scalar value, call boundary, clobbered register, and
  expected fresh source.
- Add producer or RV64 consumer support only for a complete-authority scalar
  call-boundary freshness family.
- Preserve fail-closed diagnostics for post-call scalar reads from ABI-clobbered
  registers when no freshness authority exists.

## Non-Goals

- Do not reopen large selected pointer-offset materialization from idea 634.
- Do not handle aggregate call results, sret/byval, global data, string pointer,
  branch, or move-bundle fan-in policy here.
- Do not change runtime-library behavior, expectations, unsupported markers,
  allowlists, timeouts, or pass/fail accounting.
- Do not add named-case handling for `src/ipa-sra-2.c`, `calloc`, `foo`,
  `argc`, or physical register `a0`.

## Working Model

The expected failure is a scalar freshness error across a call boundary. A value
such as `argc` may be needed after a call, but the generated RV64 code can only
reuse a post-call register if call-clobber facts and scalar freshness authority
prove that the value was preserved, republished, or rematerialized.

## Execution Rules

- Keep routine execution notes in `todo.md`.
- Prefer narrow evidence and object/disassembly/runtime proof before editing
  producer or RV64 consumer code.
- Any implementation slice must include positive coverage for the repaired
  complete-authority path and negative coverage for missing freshness authority.
- If the first owner changes to another subsystem, record the boundary in
  `todo.md` and stop for supervisor routing.
- A code-changing acceptance slice needs fresh build proof and a matching
  before/after regression comparison chosen by the supervisor.

## Steps

### Step 1: Refresh The Residual Row

Goal: confirm the current `src/ipa-sra-2.c` first observable failure.

Actions:
- Run the supervisor-selected one-row RV64 torture probe for `src/ipa-sra-2.c`.
- Record whether object compile, ELF sanity, link, and runtime stages complete.
- Capture the current C4C runtime behavior, relevant case logs, disassembly,
  and any direct rerun evidence.

Completion check:
- `todo.md` records the refreshed first owner with artifact paths and states
  whether scalar call-boundary freshness is still the active owner.

### Step 2: Trace The Freshness Boundary

Goal: identify the exact scalar value and call-boundary freshness failure.

Actions:
- Locate the prepared value consumed after the call, the call instruction, the
  physical register or home used after the call, and the call-clobber facts.
- Identify the expected fresh source: preserved value, republished value, or
  rematerialized value.
- Rule out object relocation, large-offset local memory, stack layout,
  branch/control flow, and runtime-library behavior as first owners.

Completion check:
- `todo.md` names the stale value, call boundary, clobbered location, missing
  freshness authority, and the smallest complete-authority repair family.

### Step 3: Implement One Complete-Authority Freshness Path

Goal: repair the selected scalar call-boundary freshness family without
weakening fail-closed behavior.

Actions:
- Add producer or RV64 consumer support only where explicit authority proves
  the scalar value is fresh after the call.
- Keep unsupported or diagnostic behavior for post-call ABI-clobbered register
  reads that lack preservation, republication, or rematerialization authority.
- Add focused tests for the positive complete-authority path and a negative
  missing-authority path.

Completion check:
- The narrow build/test proof passes, and the selected row no longer fails due
  to stale post-call scalar consumption.

### Step 4: Validate Boundaries

Goal: prove the slice did not overfit the named residual or weaken call-clobber
policy.

Actions:
- Re-run the supervisor-selected RV64 torture subset or allowlist.
- Compare before/after logs with the regression guard when a code slice is
  ready.
- Inspect nearby call-boundary scalar cases if the diff affects shared call,
  freshness, or register-home logic.

Completion check:
- Regression proof is non-regressing, and `todo.md` records any remaining rows
  assigned to separate owners.

### Step 5: Final Lifecycle Review

Goal: decide whether the source idea is complete or needs a follow-up split.

Actions:
- Compare the final behavior against the source idea acceptance criteria and
  reviewer reject signals.
- Close the idea only if a complete-authority scalar call-boundary freshness
  path was accepted or the precise missing authority has been durably recorded.
- Create a follow-up idea for any separate initiative discovered during the
  run.

Completion check:
- Plan owner can close, deactivate, or split the lifecycle state with matching
  validation evidence.
