# AArch64 Local-Slot Address Offset Probe Runbook

Status: Active
Source Idea: ideas/open/67_aarch64_local_slot_address_offset_probe.md

## Purpose

Determine whether the null `local_slot_address_frame_offset` path is dead,
live, or an intentionally disabled gap before attempting deletion,
implementation, or cleanup.

## Goal

Produce narrow caller/runtime evidence for the local-slot address offset path
and record the next route: delete dead code, implement missing prepared offset
coverage, or keep the disabled path documented.

## Core Rule

This is an evidence-gathering plan. Do not remove the null path, mutate shared
frame-address authority, weaken tests, or bundle unrelated dispatch-family
contraction before proving the exact path status.

## Read First

- `ideas/open/67_aarch64_local_slot_address_offset_probe.md`
- `src/backend/mir/aarch64/codegen/dispatch_publication.cpp`
  `local_slot_address_frame_offset`
- Adjacent `local_aggregate_address_frame_offset` behavior.
- Frame-address materialization and local-slot address publication consumers.
- Existing focused backend/AArch64 tests around local aggregate addresses,
  frame-slot address spelling, and local-slot publication.

## Current Targets

- `local_slot_address_frame_offset` in
  `src/backend/mir/aarch64/codegen/dispatch_publication.cpp`.
- Adjacent local aggregate address and frame-address materialization helpers.
- Local-slot address publication paths and their callers.
- Narrow probes or tests that can prove the path dead, live, or deliberately
  disabled.

## Non-Goals

- Do not contract unrelated dispatch-family helpers.
- Do not remove the null path without caller or runtime proof.
- Do not mutate shared frame-address authority before proving the exact gap.
- Do not mark tests unsupported or weaken expectations to avoid the path.
- Do not claim capability progress from classification-only evidence.

## Working Model

The current path appears adjacent to local aggregate address and frame-address
materialization, but existing audit evidence does not prove whether callers can
reach the null implementation. The plan should first map callers and runtime
conditions, then add or run the narrowest probe needed to classify the path.

The outcome must be explicit. If the path is dead, record why deletion is the
next route. If it is live, record the missing implementation surface and proof
needed. If intentionally disabled, document the disabled contract and avoid
pretending capability improved.

## Execution Rules

- Start with caller and condition inventory before editing code.
- Keep the packet narrow to local-slot address offset evidence.
- Compare local-slot behavior with adjacent aggregate/frame-address behavior.
- Prefer existing tests or minimal probes that directly exercise the path.
- If adding a probe, make it evidence-oriented and avoid expectation
  downgrades.
- Run `git diff --check` for any code/test change.
- Run the supervisor-selected focused backend/AArch64 proof after any
  code/test change.

## Ordered Steps

### Step 1: Inventory Caller And Condition Paths

Goal: determine where `local_slot_address_frame_offset` is called from and
which input conditions could reach its null result.

Primary target:
`src/backend/mir/aarch64/codegen/dispatch_publication.cpp`.

Actions:
- Inspect `local_slot_address_frame_offset` and adjacent
  `local_aggregate_address_frame_offset`.
- List direct callers and the local-slot address publication paths that feed
  them.
- Identify required prepared facts, frame-slot facts, value homes, and address
  materialization inputs.
- Compare local-slot and local-aggregate behavior to find the exact missing or
  unreachable branch.
- Record the inventory, likely path classification, and first probe packet in
  `todo.md` without implementation edits.

Completion check:
- `todo.md` records caller paths, reachability conditions, adjacent aggregate
  comparison, and the narrow proof needed to classify the null path.

### Step 2: Build A Narrow Reachability Probe

Goal: prove whether the null local-slot address offset path can be reached by
current callers.

Primary targets:
focused backend/AArch64 tests or a narrow existing route that exercises
local-slot address publication.

Actions:
- Add or use the smallest probe that reaches the local-slot address path.
- Avoid weakening supported-path expectations or marking a case unsupported.
- Keep any instrumentation or assertion focused on path classification.
- Preserve adjacent aggregate/frame-address behavior.

Completion check:
- The probe shows whether current callers reach the null path, with no
  unrelated contraction or shared authority changes.

### Step 3: Classify The Path Outcome

Goal: record the path as dead, live, or intentionally disabled based on the
probe evidence.

Primary targets:
`todo.md`, plus only narrow code/test updates if the probe itself needs
cleanup.

Actions:
- If dead, record the caller proof that makes deletion safe as the next route.
- If live, record the missing prepared offset coverage or implementation gap.
- If intentionally disabled, record the durable disabled-path contract.
- Do not implement broad frame-address authority changes in this step.
- Keep the result distinct from capability progress unless behavior actually
  changes.

Completion check:
- `todo.md` states the classification and the exact next route:
  delete dead code, implement missing prepared offset coverage, or keep the
  disabled path documented.

### Step 4: Apply Only The Classification-Sized Follow-Up

Goal: perform the smallest action justified by the evidence, if the supervisor
delegates it within this idea.

Primary targets:
the local-slot address helper, narrow tests, or documentation notes directly
linked to the classified outcome.

Actions:
- Delete dead code only if Step 2/3 proved it unreachable.
- Implement missing prepared offset coverage only if the live path and exact
  gap are proven.
- Otherwise document/guard the disabled path without widening scope.
- Do not mutate shared frame-address authority unless the classification
  explicitly proves it is the required minimal fix.

Completion check:
- The follow-up matches the classification and does not include unrelated
  dispatch contraction or testcase-shaped shortcuts.

### Step 5: Prove Local-Slot Offset Classification

Goal: validate the evidence and any classification-sized follow-up.

Primary targets:
focused backend/AArch64 tests around local-slot addresses and adjacent
aggregate/frame-address behavior.

Actions:
- Run the supervisor-delegated proof command.
- Run `git diff --check`.
- Confirm proof covers local-slot address cases likely to reach the path and
  adjacent aggregate/frame-address behavior.
- Scan the diff for unrelated contraction, unsupported downgrades, shared
  authority mutation, or named-case-only probes.
- Escalate validation if shared frame-address authority or public helper
  contracts changed.

Completion check:
- The path is classified with proof, the next route is explicit, and no
  unrelated cleanup or overfit tactic is bundled into the result.
