# RV64 Object Route Stack Parameter ABI Residual Runbook

Status: Active
Source Idea: ideas/open/644_rv64_object_route_stack_parameter_abi_residual.md

## Purpose

Classify and repair the RV64 object-route parameter ABI residual exposed by
`src/20001017-1.c`, where the first owner is now `unsupported_call_abi` after
branch stack clobber-safety is no longer the blocking authority.

## Goal

Move one semantic RV64 object-route stack-parameter ABI/home path past
`unsupported_call_abi`, or record the exact unsupported prepared ABI/home shape
with current evidence.

## Core Rule

Do not reconstruct parameter placement from source syntax, stack offsets, or
final assembly; consume explicit prepared ABI/home facts and keep unsupported
parameter homes fail-closed.

Route repair checkpoint: `review/reviewA.md` rejected the current callee-side
route because RV64 recomputed incoming stack-passed formal offsets from formal
order, ABI size/alignment, and `stack_frame_bytes`. Step 3 must replace that
route with explicit prepared incoming stack-parameter/home authority, or
classify the missing authority as a producer/prealloc publication gap before
more RV64 consumer support lands.

## Read First

- ideas/open/644_rv64_object_route_stack_parameter_abi_residual.md
- ideas/closed/374_rv64_object_route_non_register_param_homes.md
- ideas/closed/512_stack_passed_parameter_home_publication.md
- ideas/closed/635_prepared_branch_stack_clobber_safety_authority.md
- docs/rv64_gcc_torture_post_contract/infrastructure_bucket_evidence.md

## Current Scope

- Refresh the `src/20001017-1.c` RV64 residual and identify the exact
  unsupported parameter ABI/home shape.
- Compare the observed shape against closed stack-parameter and non-register
  parameter-home work before editing producer or RV64 consumer code.
- Add support only from explicit prepared ABI/home facts.
- Preserve fail-closed diagnostics for unsupported, missing, ambiguous, or
  layout-only parameter homes.

## Non-Goals

- Do not reopen branch stack-source freshness or clobber-safety authority.
- Do not handle RV64 terminator fragment lowering in this runbook.
- Do not perform generic call lowering rewrites.
- Do not change runtime behavior, expectations, unsupported markers,
  allowlists, timeouts, or pass/fail accounting.
- Do not add named-case handling for `src/20001017-1.c`.

## Working Model

The current residual is expected to be an RV64 object-route parameter admission
failure. The backend object route can currently consume supported GPR homes and
prepared FPR register homes, but the selected row needs a non-register or stack
parameter home to be represented through explicit prepared facts rather than
layout inference.

## Execution Rules

- Keep routine execution notes in `todo.md`.
- Start with a fresh one-row probe before changing code.
- Treat a changed first owner as lifecycle evidence, not automatic progress.
- Any code slice must include positive coverage for the repaired prepared
  ABI/home path and negative coverage for unsupported parameter homes.
- A code-changing acceptance slice needs fresh build proof and a matching
  before/after regression comparison chosen by the supervisor.
- Stop for supervisor routing if the first owner moves outside RV64 object-route
  parameter ABI admission.

## Steps

### Step 1: Refresh The Residual Row

Goal: confirm the current first observable owner for `src/20001017-1.c`.

Actions:
- Run the supervisor-selected one-row RV64 torture probe for `src/20001017-1.c`.
- Record whether object compile, ELF sanity, link, and runtime stages complete.
- Capture the current diagnostic text, case logs, and any prepared dump needed
  to identify the unsupported ABI/home shape.

Completion check:
- `todo.md` records the refreshed first owner and states whether
  `unsupported_call_abi` remains the active boundary.

### Step 2: Classify The Parameter Home Shape

Goal: identify the exact prepared parameter home shape that RV64 object-route
admission rejects.

Actions:
- Inspect prepared call/value/storage/frame facts for the affected function and
  parameter.
- Compare the shape with the closed non-register parameter-home and
  stack-passed parameter publication work.
- Distinguish explicit prepared ABI/home facts from stack layout, source syntax,
  MIR-only, or final assembly inference.

Completion check:
- `todo.md` names the rejected parameter, prepared home facts, missing producer
  or RV64 consumer authority, and the smallest complete repair family.

### Step 3: Implement One Prepared Incoming Stack Formal Authority Path

Goal: repair one semantic RV64 object-route parameter ABI/home path without
weakening fail-closed behavior.

Actions:
- Add producer or RV64 consumer support only where prepared facts explicitly
  describe the parameter home.
- Remove or replace any RV64 consumer path that derives incoming stack-passed
  formal load offsets from formal order, ABI size/alignment, source layout, or
  `stack_frame_bytes + incoming_offset`.
- Require an explicit prepared incoming stack-parameter/home fact for the
  caller-stack address consumed by callee-side formal loads. If no such fact is
  published, stop and route the packet to the producer/prealloc publication
  gap instead of reconstructing the offset in RV64.
- Keep local spill-slot/home coherence checks as fail-closed validation only;
  do not treat a callee local home as authority for the incoming caller-stack
  ABI offset.
- Keep unsupported diagnostics for missing, ambiguous, layout-only, or
  unsupported parameter homes.
- Add focused positive coverage for explicit incoming stack formal authority
  and negative coverage for missing or ambiguous incoming-stack authority.

Completion check:
- The narrow build/test proof passes, and `src/20001017-1.c` no longer stops at
- the repaired `unsupported_call_abi` boundary because of explicit prepared
  incoming stack formal authority, or `todo.md` records the precise
  producer/prealloc publication gap that blocks such authority.

### Step 4: Validate Boundaries

Goal: prove the slice is not named-case overfit and does not weaken parameter
ABI admission policy.

Actions:
- Re-run the supervisor-selected RV64 torture subset or allowlist.
- Compare before/after logs with the regression guard when a code slice is
  ready.
- Inspect nearby stack/non-register parameter-home cases if the diff affects
  shared call, prepared home, or RV64 object-route logic.

Completion check:
- Regression proof is non-regressing, and `todo.md` records any remaining rows
  assigned to separate owners.

### Step 5: Final Lifecycle Review

Goal: decide whether the source idea is complete or needs a follow-up split.

Actions:
- Compare final behavior against the source idea acceptance criteria and
  reviewer reject signals.
- Close the idea only if the selected row advanced through a semantic prepared
  ABI/home repair or was reclassified to a more precise existing owner with
  current evidence.
- Create a follow-up idea for any separate initiative discovered during the
  run.

Completion check:
- Plan owner can close, deactivate, or split the lifecycle state with matching
  validation evidence.
