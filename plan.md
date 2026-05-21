# Backend Regex Failure Family Inventory Runbook

Status: Active
Source Idea: ideas/open/295_backend_regex_failure_family_inventory.md

## Purpose

Reactivate the backend regex inventory after focused owner idea 369 closed,
then classify the remaining backend-regex residuals before any new
implementation work starts.

## Goal

Produce a current, source-backed residual inventory from the main build backend
regex surface and split or select one focused semantic owner for the next
implementation plan.

## Core Rule

Do not implement fixes under this umbrella plan. Use it only to capture,
classify, compare, and split or select focused owner work.

## Read First

- `ideas/open/295_backend_regex_failure_family_inventory.md`
- the latest canonical `test_after.log`, if it covers the backend regex
  surface after idea 369 closed
- recent closed owner notes when a residual appears to contradict a closure
  boundary

## Current Scope

- Main build backend regex surface:
  `ctest --test-dir build -j10 -R backend --output-on-failure`
- Local backend/unit/CLI tests selected by the backend regex
- External `c_testsuite_aarch64_backend_*` residuals
- Recently parked backend-regex buckets recorded in idea 295 after the
  idea 369 split

## Non-Goals

- Do not make implementation edits.
- Do not change expectations, unsupported classifications, allowlists, runner
  behavior, timeout policy, proof-log policy, or CTest registration.
- Do not treat the backend regex result as one monolithic failure bucket.
- Do not reopen a closed owner from counts alone.
- Do not split a new idea from a named testcase without semantic owner
  evidence.
- Do not reactivate parked focused ideas unless their current notes identify
  unsatisfied executable scope.

## Working Model

The umbrella inventory is active only long enough to find the next focused
owner. A focused owner may be a new `ideas/open/*.md` split or an existing open
idea whose current lifecycle notes still make it executable. Once that owner
is selected, the lifecycle should switch away from this umbrella before code
edits begin.

## Execution Rules

- Prefer existing canonical logs when they are fresh enough and their command
  scope matches the backend regex surface.
- If a fresh backend regex run is needed, use the main build tree command from
  the source idea.
- Separate local backend failures from external AArch64 c-testsuite failures.
- Classify by first bad semantic or generated-code owner, not by filename,
  emitted instruction text, or pass-count movement.
- Treat timeout or output-storm cases as their own bucket and avoid broad
  reruns without cleanup and timeout discipline.
- Preserve source-idea durability: routine inventory details belong in
  `todo.md`; only durable split/deactivation notes should later update the
  source idea.

## Steps

### Step 1: Reconstruct the Current Backend Regex Surface

Goal: establish the current residual set after idea 369 closed.

Primary target: `test_after.log` or a fresh main-build backend regex run.

Actions:

- Inspect the existing canonical proof logs for command scope and freshness.
- If no matching backend-regex log exists, ask the supervisor to provide or
  authorize the exact backend-regex capture command before broad execution.
- Record selected, passed, failed, and timed-out counts in `todo.md`.
- List non-passing tests by local backend bucket versus external
  `c_testsuite_aarch64_backend_*` bucket.

Completion check:

- `todo.md` names the log source or required capture command and records the
  current backend-regex residual list.

### Step 2: Classify Residuals by Semantic Owner

Goal: identify tractable failure families without testcase overfit.

Primary target: residual test outputs, generated artifacts, semantic BIR,
prepared BIR, and recent closure notes.

Actions:

- Group residuals by first bad boundary: semantic admission, prepared handoff,
  selected-machine printing, AArch64 lowering, ABI publication, memory
  writeback, runtime mismatch, timeout, or output storm.
- Compare candidate groups against recent closed owners before declaring a
  reopen.
- Mark parked or closure-deferred open ideas as non-executable unless their
  lifecycle notes identify an active unresolved scope.
- Reject groups whose only evidence is a shared filename pattern or pass-count
  movement.

Completion check:

- `todo.md` records the classified buckets, the evidence for each viable owner,
  and the rejected adjacent owners.

### Step 3: Select the Next Focused Owner

Goal: choose one focused semantic repair route for lifecycle handoff.

Primary target: the strongest current bucket from Step 2.

Actions:

- Prefer an existing open idea only if its current lifecycle notes still
  describe unsatisfied executable scope.
- If no existing idea fits, draft a new focused `ideas/open/*.md` with goal,
  scope, acceptance criteria, and reviewer reject signals.
- Keep the selected owner narrow enough for focused backend, backend-route, or
  semantic proof.
- Leave implementation details for the future focused plan.

Completion check:

- Exactly one next owner is selected, or `todo.md` explains why no owner is
  activatable from the current evidence.

### Step 4: Prepare Lifecycle Handoff

Goal: move execution away from the umbrella before implementation starts.

Primary target: `ideas/open/295_backend_regex_failure_family_inventory.md`,
`plan.md`, and `todo.md`.

Actions:

- Add a durable deactivation note to the source idea only after the selected
  owner and evidence are clear.
- Request lifecycle switch to the selected focused owner.
- Do not keep this umbrella active for code edits.

Completion check:

- The umbrella inventory has a durable handoff note, and lifecycle state is
  ready to switch to the focused owner.
