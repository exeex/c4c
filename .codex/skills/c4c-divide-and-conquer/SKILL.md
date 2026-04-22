---
name: c4c-divide-and-conquer
description: Use when the active c4c route is repeatedly colliding with the same hard problem, the first bad fact keeps moving without reducing the owned failure family, or one monolithic testcase is driving execution. This skill tells the supervisor how to split the problem into focused probes, create a new idea under ideas/open/, and switch the active plan into a decomposition-oriented runbook instead of continuing the stuck route.
---

# C4C Divide And Conquer

Use this skill when the current active route is not producing real progress and
the right fix is to decompose the problem into smaller owned seams.

This is not a replacement for `c4c-plan-owner`. It is a route-reset workflow
for the supervisor. When it triggers, the supervisor should use the resulting
idea/plan shape and then hand lifecycle edits to `c4c-plan-owner`.

## Trigger Conditions

Use this skill when one or more of these are true:

- the same `Current Step ID` keeps accumulating packets while the owned failure
  family does not shrink
- the first bad fact keeps moving inside one large testcase, but no generic
  capability claim has become provable
- one monolithic external testcase is driving the route and should be reduced
  to focused probes under `tests/backend/case/`
- recent work mostly changed `plan.md` / `todo.md` or testcase-shaped patches
  without reducing `ctest -R backend` failure surface
- the active source idea is still valid, but the current runbook shape is too
  coarse to execute safely

Do not use this skill merely because a packet failed once.

## Goal

Replace a stuck route with a decomposition initiative that:

1. identifies the separable seams inside the blocked problem
2. creates focused probes or tests for those seams
3. writes a new durable idea under `ideas/open/`
4. switches the active plan to that new decomposition idea

The result should turn one hard blocking problem into several smaller
capability-oriented packets.

## Core Rule

The decomposition initiative must describe a new execution shape, not just
restate that the old testcase is hard. If the only output would be "look at
the same testcase again later", do not switch.

## Start Here

1. Read [`AGENTS.md`](/workspaces/c4c/AGENTS.md).
2. Read the current [`plan.md`](/workspaces/c4c/plan.md),
   [`todo.md`](/workspaces/c4c/todo.md), and linked source idea.
3. Inspect the most recent proof/logs for the blocked route.
4. Inventory the visible seams that can be separated into focused probes,
   especially under [`tests/backend/case`](/workspaces/c4c/tests/backend/case).
5. Decide whether the new work is truly a separate initiative under the repo's
   single-plan lifecycle.

## Decomposition Workflow

### Step 1: Prove The Route Is Actually Stuck

Collect concrete evidence:

- repeated packets on the same step with little or no failure-family reduction
- moving first-bad-fact behavior inside one testcase
- reviewer warnings about testcase-overfit, route drift, or monolithic proof

Do not switch on vibes alone.

### Step 2: Define The Smaller Seams

Write down the smallest capability seams that can be owned independently.

Prefer seams such as:

- one byval/home publication contract
- one helper-boundary handoff contract
- one variadic ownership seam
- one return-path contract

Avoid naming seams after testcase numbers.

### Step 3: Extract Focused Probes

Prefer adding or expanding focused probes before more backend surgery:

- place backend-owned probes under `tests/backend/case/`
- keep one problem per file
- keep the original large testcase only as an integration probe

If no focused probe can yet be extracted, explain why in the new idea.

### Step 4: Create A New Idea

Create a new file under `ideas/open/` when the decomposition work is a durable
initiative rather than a minor `plan.md` clarification.

The new idea should include:

- a title that names the capability family, not the testcase number
- why the current route is blocked
- what seams are being split out
- what probes or case files will represent those seams
- what counts as progress for the decomposition initiative
- explicit boundaries against the old idea

The old idea usually stays open unless the source intent itself moved.

### Step 5: Switch The Active Plan

Ask `c4c-plan-owner` to switch from the blocked idea to the new decomposition
idea. The switched `plan.md` should be decomposition-oriented, not repair-only.

That runbook should usually follow this pattern:

1. establish the blocked family baseline
2. enumerate separable seams
3. extract focused probes under `tests/backend/case/`
4. bind each probe to an owned backend capability
5. only then return to implementation packets

## Decomposition Plan Shape

When the new runbook is generated, prefer steps like:

- `Establish The Blocked Failure-Family Baseline`
- `Split The Monolithic Probe Into Focused Cases`
- `Bind Each Focused Case To One Owned Backend Seam`
- `Resume Implementation On The Narrowest Generic Seam`

This is different from a normal repair runbook, because the first job is to
improve the shape of execution, not to patch backend code immediately.

## Hard Boundaries

1. Do not silently mutate the active source idea when the work is a separate
   initiative.
2. Do not keep chasing one testcase after deciding decomposition is required.
3. Do not create focused probes that are just slightly smaller copies of the
   same monolith; each probe must have one primary contract.
4. Do not treat the decomposition switch itself as backend progress. It is a
   route-quality correction.
5. Do not close the old idea unless its durable source intent is actually
   complete or superseded.

## Output

Return:

- whether decomposition is justified: `yes` or `no`
- the blocked route symptoms that triggered it
- the proposed new idea path under `ideas/open/`
- the seams to split out
- the expected probe files under `tests/backend/case/`
- the requested lifecycle action for `c4c-plan-owner`
  - `switch active plan`
  - `rewrite current plan only`
  - `do not switch`
