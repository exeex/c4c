# Backend Regex Failure Family Inventory Runbook

Status: Active
Source Idea: ideas/open/295_backend_regex_failure_family_inventory.md

## Purpose

Refresh and classify the backend-regex failure surface after the 00173-focused
ideas 365 and 366 closed, then identify the next focused semantic owner before
implementation work begins.

## Goal

Produce a current backend-regex inventory from the main build tree, separate
local backend failures from external AArch64 c-testsuite failures, and hand the
supervisor one focused repair owner or an explicit no-owner-yet result.

## Core Rule

Classify semantic failure families. Do not claim progress by changing
expectations, unsupported classifications, runner behavior, timeout policy,
CTest registration, proof-log policy, or by fixing one named testcase shape.

## Read First

- `ideas/open/295_backend_regex_failure_family_inventory.md`
- The latest lifecycle context: ideas 365 and 366 are closed, and `00173` now
  passes.
- Existing open ideas only when comparing candidate owners; treat parked notes
  as lifecycle evidence and do not reactivate a parked/satisfied focused idea
  from its stale header alone.

## Current Scope

- Capture a fresh backend-regex result from `/workspaces/c4c/build`.
- Classify remaining non-passing tests by semantic owner and by source
  category.
- Exclude the completed 00173 path unless fresh evidence shows a new,
  distinct first bad fact.
- Select the next focused owner from existing open ideas or prepare a
  lifecycle handoff to create one.

## Non-Goals

- Do not implement backend fixes under this umbrella plan.
- Do not reopen closed ideas 365 or 366 because of historical counts.
- Do not mutate `ideas/closed/`, review artifacts, logs, runner policy,
  expectations, unsupported classifications, or CTest registration.
- Do not broaden a parked focused idea into active work unless current
  generated-code evidence contradicts its parked boundary.

## Working Model

This source idea is an inventory umbrella. Its job is to keep broad backend
regex results from becoming a monolithic implementation bucket. The executor
should gather fresh evidence, classify the residual surface, and stop at a
focused lifecycle recommendation instead of editing compiler code.

## Execution Rules

- Use the main build tree for backend-regex evidence.
- Keep routine inventory progress in `todo.md`.
- If classification identifies a new focused owner, report the exact proposed
  idea title, representative tests, first bad facts, and reject signals for
  plan-owner handoff.
- If an existing open idea already owns the current first bad fact, identify it
  by path and explain why parked or stale alternatives are not selected.
- Escalate to the supervisor before any implementation edits.

## Ordered Steps

### Step 1: Capture Fresh Backend Regex Inventory

Goal: establish the current non-passing surface after the 00173 chain closed.

Concrete actions:

- Run the supervisor-delegated backend-regex command from the main build tree.
- Record selected, passed, failed, and timed-out counts in `todo.md`.
- List non-passing tests and separate local backend/unit/CLI tests from
  external `c_testsuite_aarch64_backend_*` tests.
- Confirm whether `c_testsuite_aarch64_backend_src_00173_c` is passing in the
  fresh result.

Completion check:

- `todo.md` contains the command, result counts, non-passing list, and the
  00173 status from the fresh inventory.

### Step 2: Classify Residual Families

Goal: group the remaining failures by semantic owner instead of by filename.

Concrete actions:

- For each residual bucket, inspect available output, generated artifacts, or
  focused dumps enough to identify the first bad fact.
- Separate runtime mismatch, runtime nonzero/crash, timeout/output-storm,
  machine-printer/prepared-node, semantic handoff, and local backend buckets.
- Compare each candidate against open focused ideas and recent closed owner
  boundaries.
- Reject buckets whose only support is historical counts or a single filename.

Completion check:

- `todo.md` records candidate buckets, representative evidence, rejected
  owners, and the strongest current semantic owner.

### Step 3: Select Or Split The Next Focused Owner

Goal: decide the next executable lifecycle target.

Concrete actions:

- If an existing open idea clearly owns the strongest current first bad fact,
  recommend activating that idea and explain the match.
- If no existing idea owns it, draft the focused owner summary needed for a new
  `ideas/open/*.md` handoff: goal, scope, out-of-scope boundaries,
  acceptance criteria, and concrete reviewer reject signals.
- If the residual surface is too ambiguous, identify the smallest extra probe
  needed before splitting.

Completion check:

- `todo.md` contains one of: selected existing idea path, new-idea handoff
  draft, or an explicit ambiguity with the next probe.

### Step 4: Supervisor Handoff

Goal: leave lifecycle state ready for the next plan-owner or executor action.

Concrete actions:

- Summarize the inventory result and selected owner in `todo.md`.
- Do not edit source ideas directly unless the supervisor delegates a
  plan-owner split after reviewing the classification.
- Recommend the narrow proof command that should accompany the next focused
  executor packet.

Completion check:

- The supervisor can either delegate the selected focused implementation
  packet or call plan-owner to switch/split without redoing the inventory.
