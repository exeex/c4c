# ABI Call Result And Stack-Frame Lowering Runbook

Status: Active
Source Idea: ideas/open/613_abi_call_result_stack_frame_lowering.md

## Purpose

Activate idea 613 as the ABI/RV64 consumer runbook after idea 612 closed.

## Goal

Repair ordinary same-module call, result, return, and supported stack-frame
RV64 lowering only when prepared call/return/frame facts already exist and the
first owner is ABI/RV64 consumption.

## Core Rule

Consume explicit prepared ABI, result, return, and frame facts; do not infer
missing call/return authority from final assembly shape, runtime behavior, or
testcase names.

## Read First

- `ideas/open/613_abi_call_result_stack_frame_lowering.md`
- `docs/rv64_gcc_torture_1000_pass_recovery/failure_bucket_map.md`
- `docs/rv64_gcc_torture_1000_pass_recovery/high_yield_followup_plan.md`
- `todo.md`

## Current Targets

- Ordinary same-module call and result rows whose first reproduced stop is
  ABI/RV64 consumption.
- Supported stack-frame layout rows with complete local/global producer
  prerequisites.
- Return move-bundle target rows when prepared return facts are present.
- Negative guard rows for variadic calls, library calls, runtime mismatch,
  missing prepared authority, local/global producer gaps, expectation-only
  changes, unsupported markers, allowlists, timeouts, and accounting.

## Non-Goals

- Do not implement variadic or library call policy.
- Do not repair runtime aborts, runtime mismatches, BIR local/global
  producers, prepared authority production, or unrelated RV64 consumer
  families.
- Do not change expectations, unsupported markers, allowlists, timeouts,
  accounting, or GCC torture classification metadata as progress.
- Do not add filename-, row-, operation-id-, or testcase-shaped shortcuts.

## Working Model

- The source idea estimates `46` ordinary call ABI/result rows, `12`
  stack-frame layout rows, and `2` return move-bundle target rows.
- This idea runs only after local-memory and global-data producer
  prerequisites are explicit enough that ABI/RV64 consumption can require
  complete upstream facts.
- Runtime, library, variadic, and missing-authority rows are diagnostics and
  guardrails, not implementation targets for this runbook.

## Execution Rules

- Start each code-changing packet from refreshed backend diagnostics or focused
  probes that name first owner, prepared facts, positive rows, and negative
  guards.
- Record the selected row family, representative positives, guard rows, and
  exact proof command in `todo.md` before implementation.
- Each code-changing step must run at least:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`
- Escalate to reviewer if a diff weakens unsupported contracts, rewrites
  expectations, handles runtime/library/variadic policy, consumes missing
  authority, or proves progress only by matching a named source file.

## Step 1: Refresh ABI Consumer Ownership

Goal: identify current ordinary call/result/frame/return rows that still
belong to idea 613.

Primary target:
- Backend diagnostics and focused probes for `unsupported_call_abi`,
  stack-frame layout, return move-bundle target, and adjacent ABI/RV64
  consumer stops.

Actions:
- Refresh or inspect current backend diagnostics for ordinary call/result,
  stack-frame layout, and return move-bundle candidates.
- Split candidates by first owner:
  - ordinary same-module ABI/RV64 consumer with complete prepared facts
  - supported stack-frame layout consumer with complete prerequisites
  - return move-bundle target consumer with prepared return facts
  - variadic, library, runtime, missing prepared-authority, local/global
    producer, or unrelated downstream owner
- Pick the first implementation packet with same-family breadth and clear
  guard rows.
- Record selected Step 2 packet, positives, negatives, and proof command in
  `todo.md`.

Completion check:
- `todo.md` names the refreshed ABI groups, selected Step 2 family, positive
  rows or a no-breadth blocker, negative guard rows, and exact proof command.

## Step 2: Implement First Ordinary ABI Consumer

Goal: lower the first general ordinary call/result ABI consumer family that has
complete prepared facts.

Primary target:
- RV64/MIR ABI consumer path selected by Step 1.

Actions:
- Locate the RV64 consumer rejection for the selected ordinary call/result
  family.
- Add the smallest semantic lowering rule shared by the selected rows.
- Require explicit prepared call/result authority, operand locations, widths,
  return locations, and stack/register boundary facts.
- Preserve fail-closed diagnostics for missing authority, variadic/library
  policy, runtime rows, producer gaps, unsupported widths, and unrelated
  fragment classes.

Completion check:
- Multiple ordinary call/result rows compile or move past their ABI consumer
  stop.
- Negative guard rows remain under their original owners.
- Backend subset proof passes.

## Step 3: Add Supported Frame Or Return Handling

Goal: repair the next in-scope stack-frame layout or return move-bundle target
consumer exposed by Step 1/Step 2.

Primary target:
- Supported frame layout or prepared return target consumer rows with complete
  upstream facts.

Actions:
- Re-run focused residual probes after Step 2.
- Select a supported frame or return family with shared authority and more
  than testcase-only breadth when possible.
- Add only the consumer handling required by explicit prepared facts.
- Keep local/global producer repair, prepared authority production, runtime,
  library/variadic policy, and move-bundle authority production out of scope.

Completion check:
- At least one stack-frame or return row moves past its ABI/RV64 consumer stop
  when available.
- Call/result positive rows from Step 2 remain green.
- Negative proof still covers missing-authority and policy rows.
- Backend subset proof passes.

## Step 4: Broaden Within ABI Consumer Authority

Goal: extend support only to adjacent ordinary ABI consumer shapes that share
the proven Step 2/Step 3 authority model.

Actions:
- Refresh residual ABI diagnostics after the first implementation packets.
- Identify adjacent ordinary call/result/frame/return shapes with complete
  prepared facts.
- Add narrowly scoped consumer support for same-authority variants.
- Reclassify out-of-scope residuals to their owning routes without changing
  expectations or unsupported markers.

Completion check:
- The ordinary ABI consumer population is materially reduced or classified to
  concrete downstream owners.
- Guard rows remain separated.
- Backend subset proof passes.

## Step 5: Residual Split Or Close-Readiness Classification

Goal: decide whether idea 613 is complete, needs another ABI consumer packet,
or should split residuals into separate open ideas.

Actions:
- Refresh remaining ordinary call/result/frame/return residuals.
- Classify residuals by first owner and prepared-authority completeness.
- Identify any durable follow-up idea needed for variadic/library policy,
  missing prepared facts, runtime mismatch, local/global producer gaps, or
  move-bundle authority production.
- Record close readiness, next-family recommendation, or split need in
  `todo.md`.

Completion check:
- `todo.md` states whether idea 613 is close-ready, should continue with a
  named ABI consumer family, or should split/retire the route.
- The recommendation is backed by current diagnostics and backend subset proof
  when code changed in the route.
