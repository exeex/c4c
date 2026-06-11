# Phase E Route 5 Edge Join-Source View Consumer Migration

Status: Active
Source Idea: ideas/open/183_phase_e_route5_edge_join_source_view_consumer_migration.md

## Purpose

Prove one bounded AArch64 edge-copy or join parallel-copy source-recovery
consumer can read Route 5 edge/join-source semantic records while preserving
prepared helper fallback and oracle behavior.

## Goal

Switch one selected AArch64 edge-copy or join-source reader to prefer Route 5
semantic join-source records when present, without moving prepared placement or
move-bundle policy into BIR.

## Core Rule

Route 5 may provide the semantic source identity for the selected consumer, but
prepared edge-copy and move-bundle helpers remain the fallback/oracle surfaces.

## Read First

- ideas/open/183_phase_e_route5_edge_join_source_view_consumer_migration.md
- docs/bir_prealloc_fusion/phase_d_mir_consumer_switch_plan.md
- Existing AArch64 edge-copy and join parallel-copy source-recovery code.
- Existing Route 5 BIR/prepared route-index and fail-closed tests.

## Current Scope

- One AArch64 edge-copy or join parallel-copy source-recovery consumer.
- Route 5 records keyed by edge, predecessor, destination block, and copied
  value role.
- Missing-predecessor, no-source, memory-source, and absent-route behavior.
- Prepared edge-publication, edge-publication source-producer, current-block
  join-source, and move-bundle helpers as fallback/oracle references.

## Non-Goals

- Do not move parallel-copy scheduling, out-of-SSA placement, source or
  destination homes, move-bundle selection, branch spelling, or final edge-copy
  records into BIR.
- Do not hide or delete prepared edge-copy helpers after this one consumer
  migration.
- Do not perform broad aggregate contraction.
- Do not rescan predecessors or BIR nodes as a substitute for Route 5 owned
  records.

## Working Model

- Route 5 is the semantic view boundary for edge/join-source identity.
- Prepared data remains the policy and fallback authority for edge-copy
  mechanics.
- Tests must prove route/prepared equivalence and fail-closed behavior rather
  than only a selected happy path.

## Execution Rules

- Keep each implementation step narrow and behavior-preserving.
- Prefer existing helper names and local conventions over new abstraction
  layers.
- Preserve prepared fallback for absent, incomplete, or invalid Route 5 data.
- For code-changing steps, run a fresh build plus the focused AArch64 and
  Route 5/prepared tests selected by the supervisor.
- Treat expectation downgrades, helper-only reshapes, or named-case shortcuts
  as route drift.

## Step 1: Select Consumer And Baseline Behavior

Goal: choose the single AArch64 reader boundary and record the current prepared
helper behavior it must preserve.

Actions:

- Inspect the AArch64 edge-copy and join parallel-copy source-recovery readers.
- Select one consumer whose Route 5 boundary is local and testable.
- Identify the prepared helper(s) that remain fallback/oracle surfaces.
- Identify focused tests that already cover the selected path and nearby
  fail-closed behavior.
- Propose the narrow build/test proof command for the implementation steps.

Completion Check:

- `todo.md` names the selected consumer, prepared fallback/oracle helpers,
  focused test files, and proposed proof command.
- No production or test behavior changes are made in this step.

## Step 2: Add Or Expose Route 5 View Boundary

Goal: provide a local Route 5 query/facade boundary for the selected consumer
without changing fallback behavior.

Actions:

- Add or expose the minimal Route 5 access path needed by the selected reader.
- Keep route record lookup keyed by edge, predecessor, destination block, and
  copied value role.
- Make absent, incomplete, or mismatched Route 5 data fail closed to prepared
  fallback.
- Avoid moving prepared edge-copy or move-bundle policy into BIR.

Completion Check:

- The selected consumer can consult the Route 5 boundary through a narrow local
  interface.
- Existing prepared behavior remains the observable fallback.
- Focused build and test proof passes.

## Step 3: Prefer Route 5 In The Selected Consumer

Goal: switch the selected reader to use Route 5 semantic source records when
available and valid.

Actions:

- Route the selected source-recovery decision through the Route 5 boundary.
- Preserve prepared fallback for absent-route, missing-predecessor, no-source,
  and memory-source cases.
- Keep prepared helpers as the oracle for policy and mechanical edge-copy
  behavior.
- Avoid broad call-site churn outside the selected consumer.

Completion Check:

- The selected reader prefers valid Route 5 semantic source records.
- Prepared fallback behavior remains equivalent for unsupported or absent route
  cases.
- Focused build and test proof passes.

## Step 4: Add Route/Prepared Equivalence Coverage

Goal: prove the selected Route 5 migration is not a happy-path-only change.

Actions:

- Add or tighten tests for normal predecessor behavior.
- Cover missing-predecessor, no-source, memory-source, and absent-route cases.
- Preserve existing route-index fail-closed coverage.
- Do not weaken existing oracle expectations.

Completion Check:

- Tests cover route/prepared equivalence across the required cases.
- Focused build and test proof passes.
- No unsupported expectation downgrade or helper-only rename is claimed as
  progress.

## Step 5: Handoff And Closure Readiness

Goal: prepare the completed migration for supervisor review and lifecycle
closure.

Actions:

- Summarize the selected consumer, Route 5 boundary, fallback/oracle behavior,
  and tests in `todo.md`.
- Record the final focused proof command and result.
- Call out any remaining prepared helper surfaces as intentional non-goals.
- State that broad prepared API deletion and aggregate contraction remain out
  of scope.

Completion Check:

- `todo.md` contains a concise handoff summary and proof result.
- The active idea acceptance criteria are satisfied.
- The supervisor has enough evidence to request review or lifecycle closure.
