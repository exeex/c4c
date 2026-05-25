# AArch64 Calls Emission Consolidation Follow-Up Runbook

Status: Active
Source Idea: ideas/open/02_aarch64_calls_emission_consolidation.md

## Purpose

Continue the AArch64 call-emission consolidation after the Step 5 lifecycle
review found the source idea is not yet complete.

## Goal

Remove the remaining AArch64-local call-planning reconstruction paths and leave
each surviving `calls*.cpp` helper boundary explicitly emission-only.

## Core Rule

Every step must remove duplicate planning authority or clarify an
emission-only boundary. Do not count helper renames, pure file reshuffling, or
expectation weakening as progress.

## Review Finding

The prior runbook completed its first ownership pass, including moving
prepared-clobber effect construction out of `calls_printing.cpp`. The source
idea remains open because the current AArch64 call family still contains a
large helper surface and several BIR call metadata reads that may duplicate
prepared call-plan authority:

- `calls_common.cpp` computes outgoing stack bytes from retained `CallInst`
  ABI data alongside `PreparedCallPlan` argument data.
- `calls_argument_sources.cpp` checks retained call argument type/byval facts
  while selecting call argument sources.
- `calls_byval_aggregates.cpp` rechecks retained `arg_abi` shape for byval
  aggregate handling.
- `calls_moves.cpp` still has synthetic fallback bundles and small-byval
  stack-lane logic that must be proven prepared-fact driven or removed.
- `calls_dispatch_bridge.cpp` remains a large bridge surface and should be
  audited only for call-boundary ownership, not broad dispatch cleanup.

## Read First

- `ideas/open/02_aarch64_calls_emission_consolidation.md`
- `src/backend/mir/aarch64/codegen/calls.hpp`
- `src/backend/mir/aarch64/codegen/calls.cpp`
- `src/backend/mir/aarch64/codegen/calls_common.cpp`
- `src/backend/mir/aarch64/codegen/calls_argument_sources.cpp`
- `src/backend/mir/aarch64/codegen/calls_byval_aggregates.cpp`
- `src/backend/mir/aarch64/codegen/calls_moves.cpp`
- `src/backend/mir/aarch64/codegen/calls_preservation.cpp`
- `src/backend/mir/aarch64/codegen/calls_dispatch_bridge.cpp`
- `src/backend/mir/aarch64/codegen/calls_printing.cpp`
- `src/backend/mir/aarch64/codegen/README.md`
- Focused backend call tests under `tests/backend/mir/`

## Current Targets / Scope

- Remaining AArch64-local reads of retained `bir::CallInst` argument ABI/type
  metadata inside `calls*.cpp`.
- Prepared-fact consumption around byval aggregates, stack argument sizing,
  preservation republication, and call-boundary moves.
- `calls*.hpp` declarations that expose obsolete helper boundaries.
- Build metadata when a helper file can be retired.
- Focused tests proving the prepared plan owns the call-boundary fact being
  consolidated.

## Non-Goals

- Do not invent a new shared call-plan API as part of this runbook.
- Do not perform broad dispatch responsibility cleanup; that belongs outside
  this source idea.
- Do not change behavior solely to reduce line count.
- Do not move AArch64-specific emission details into the shared planner.
- Do not weaken unsupported/expected-output contracts to make consolidation
  appear complete.

## Working Model

- Shared prepared call-plan facts own call-planning decisions.
- AArch64 call code may still inspect retained BIR only to validate identity,
  emit target-specific machine forms, or report diagnostics when prepared facts
  and retained BIR disagree.
- If a required fact is missing from prepared data, stop and record the missing
  authority in `todo.md` instead of rebuilding the planner locally.
- Helper files are acceptable only when their names and interfaces describe
  real emission-only ownership.

## Execution Rules

- Start each code-changing step with an audit of the named helper family and
  its direct tests.
- Prefer deleting local reconstruction logic over wrapping it in a new helper.
- Keep each slice small enough for a fresh build plus focused backend proof.
- Escalate to broader backend validation after retiring a file, moving a
  helper across module boundaries, or changing call-boundary effect ordering.
- Reject testcase-shaped shortcuts, expectation downgrades, and pure
  classification-only changes claimed as capability progress.

## Step 1: Audit Residual Prepared-Fact Gaps

Goal: identify the next remaining AArch64-local call-planning reconstruction
path that can be removed without broad dispatch cleanup.

Primary targets:

- `calls_common.cpp`
- `calls_argument_sources.cpp`
- `calls_byval_aggregates.cpp`
- `calls_moves.cpp`
- `calls_preservation.cpp`
- `calls_dispatch_bridge.cpp`

Actions:

- List each retained `bir::CallInst`, `arg_abi`, or `arg_types` read in the
  target files.
- Classify each read as emission validation, diagnostic identity, duplicate
  planning authority, or missing prepared-fact dependency.
- Select one coherent duplicate-authority family for the next executor packet.
- Record the selected family, expected deletion/move path, and focused proof
  command in `todo.md`.

Completion check:

- `todo.md` names one next consolidation family and the proof scope, with no
  source-idea edits required.

## Step 2: Remove One Residual Reconstruction Path

Goal: replace the selected local reconstruction path with prepared-fact
consumption, or stop with a precise missing-prepared-fact note if no prepared
authority exists yet.

Actions:

- Delete or bypass the selected duplicate local decision.
- Keep retained BIR checks only when they validate identity or produce a
  diagnostic for inconsistent prepared data.
- Update focused tests only to preserve or strengthen the ownership contract.
- Run `cmake --build --preset default` plus the focused backend tests selected
  in Step 1.

Completion check:

- The selected path no longer owns call-planning authority locally, and
  `test_after.log` records passing build plus focused proof.

## Step 3: Consolidate the Affected Helper Boundary

Goal: remove or rename the helper/file boundary made obsolete by Step 2.

Actions:

- Merge helper declarations back into the surviving owner when the helper no
  longer describes a real emission-only boundary.
- Delete retired source/header files and update build metadata when a file is
  fully obsolete.
- Keep `calls_dispatch_bridge.*` changes limited to call-boundary bridge
  ownership; do not absorb unrelated dispatch work from another idea.
- Run a fresh build and focused backend proof after each coherent file
  retirement or public-header change.

Completion check:

- The affected helper boundary is either retired or explicitly emission-only,
  and build metadata/include graphs match the new boundary.

## Step 4: Broader Backend Checkpoint

Goal: prove the latest consolidation checkpoint did not regress adjacent call
or printer behavior.

Actions:

- Run the supervisor-selected broader backend validation scope.
- Include focused AArch64 call-boundary, machine-printer, prepared call
  boundary, and any affected x86 shared-boundary tests when the change touches
  shared prepared-call behavior.
- Record exact proof commands and results in `todo.md`.

Completion check:

- The broader checkpoint passes, or `todo.md` records a precise blocker tied
  to the active source idea.

## Step 5: Closure Review

Goal: decide whether the source idea is complete after the follow-up
checkpoint or whether another runbook checkpoint is needed.

Actions:

- Recheck the final `calls*.cpp`/`calls*.hpp` file set against the source idea
  acceptance criteria.
- Confirm target-local calls code no longer rederives call-plan decisions
  already present in shared prepared facts.
- Confirm surviving helper files have explicit emission-only ownership.
- If durable remaining work exists, keep the idea open and request another
  runbook checkpoint instead of closing.

Completion check:

- Lifecycle state gives the supervisor an unambiguous close/keep-active
  decision.
