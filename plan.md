# Backend Regex Failure Family Inventory Runbook

Status: Active
Source Idea: ideas/open/295_backend_regex_failure_family_inventory.md
Activated from: remaining open umbrella inventory after focused idea 307 closed

## Purpose

Continue the backend-regex umbrella inventory after the closed focused AArch64
owners, using the post-307 evidence as the starting point for the next
classification pass.

## Goal

Classify the remaining backend-regex failures well enough to split the next
focused semantic repair idea, or record why no focused owner is ready.

## Core Rule

This umbrella runbook is inventory and classification only. Do not implement
repairs or improve counts by changing expectations, allowlists, unsupported
classifications, CTest registration, timeout policy, runner behavior, proof
logs, or testcase-specific matching.

## Read First

- `ideas/open/295_backend_regex_failure_family_inventory.md`
- `test_before.log` and `test_after.log`, noting that the current canonical
  proof logs are focused idea 307 closure logs, not a fresh broad backend-regex
  inventory
- Generated AArch64 artifacts under `build/c_testsuite_aarch64_backend/` when
  needed for classification
- Closed-owner boundaries 285 through 307 only as historical context when a
  residual appears to overlap a completed owner

## Current Scope

- Main-build backend regex inventory from `/workspaces/c4c/build`
- Local backend/unit/CLI failures if any remain in the backend regex scope
- `c_testsuite_aarch64_backend_*` compile, assembler, runtime, mismatch,
  crash, timeout, and output-storm residuals
- Candidate focused owner splits for semantic backend capability gaps
- The `00182.c` `RUNTIME_MISMATCH` residual, which returned to this umbrella
  after idea 307 removed the old illegal large-immediate assembler failure

## Non-Goals

- No implementation edits.
- No expectation, allowlist, unsupported, registration, timeout, runner,
  proof-log, or CTest-registration edits.
- No reopening closed owners 285 through 307 from failing counts alone.
- No monolithic repair owner for all `ctest -R backend` failures.
- No broad runtime rerun without timeout handling and stale-process cleanup.

## Working Model

- Idea 307 closed the old `00182.c` assembler-stage large scalar immediate
  failure. The current focused proof shows `00182.c` now fails as
  `RUNTIME_MISMATCH`; that residual belongs back in this umbrella runtime
  bucket unless a later probe proves a focused semantic owner.
- The latest broad backend-regex inventory recorded in idea 295 before idea
  307 selected 352 tests, passed 306, and failed 46, all under
  `c_testsuite_aarch64_backend_*`. Treat that as historical context until a
  supervisor-delegated fresh broad capture or accepted broad log is available.
- Separate source buckets before proposing owner splits: local backend tests,
  AArch64 compile/machine-printer failures, assembler failures, semantic
  `lir_to_bir` admission failures, runtime nonzero exits, runtime mismatches
  or crashes, timeouts, and output storms.
- A focused split must name a semantic owner and the evidence for that owner.
  Shared filenames, shared pass-count movement, or one diagnostic string alone
  is not enough.

## Execution Rules

- Keep routine classification notes in `todo.md`.
- If a focused semantic owner is found, create a separate `ideas/open/*.md`
  owner and switch lifecycle state before implementation begins.
- If the best result is still uncertain, leave idea 295 active and record the
  blocking evidence gap in `todo.md`.
- Preserve closed-owner boundaries. Reopen or supersede a closed owner only
  with generated-code or proof evidence that contradicts its closure boundary.
- For any broad runtime proof, use bounded commands with stale-process cleanup
  as directed by the supervisor.

## Ordered Steps

### Step 1: Reconstruct Post-307 Backend-Regex Context

Goal: establish the usable residual context after focused idea 307.

Primary targets: `ideas/open/295_backend_regex_failure_family_inventory.md`,
`test_before.log`, and `test_after.log`

Actions:

- Read the post-307 closure note in idea 295 and the focused idea 307 proof
  logs.
- Record that `00182.c` moved from illegal large-immediate assembly to
  `RUNTIME_MISMATCH` and has returned to the runtime mismatch bucket.
- Determine whether the next classification packet can proceed from existing
  accepted evidence or needs a supervisor-delegated fresh bounded
  `ctest -R backend` capture.
- Do not run a fresh broad backend regex command unless the supervisor
  explicitly assigns that proof command.

Completion check:

- `todo.md` records the current post-307 classification context, what evidence
  source was used, and whether a fresh broad capture is required before Step 2.

### Step 2: Classify Residual Buckets

Goal: separate residuals by failure source and likely semantic owner.

Primary target: `build/c_testsuite_aarch64_backend/`

Actions:

- For compile or machine-printer failures, inspect diagnostics and generated
  artifacts needed to identify the failing operation shape.
- For assembler failures, inspect emitted assembly enough to name the illegal
  operand, relocation, or address form.
- For runtime nonzero, mismatch, crash, timeout, or output-storm failures,
  keep them distinct unless generated-code evidence proves a shared backend
  capability.
- Compare likely overlaps against closed owners 285 through 307 before
  proposing any reopen or related split.

Completion check:

- `todo.md` contains a bucketed residual list with evidence notes for each
  candidate family and explicit parked buckets for uncertain cases.

### Step 3: Choose the Next Focused Owner

Goal: decide whether a tractable semantic repair family is ready to split.

Actions:

- Prefer the largest crisp bucket with a shared semantic backend capability and
  narrow proof surface.
- Reject testcase-shaped owners, exact filename matching, instruction-string
  matching, and expectation-only progress.
- If no owner is ready, record the missing evidence and the next narrow probe
  needed for classification.

Completion check:

- Either a focused owner candidate is named with tests, semantic scope,
  boundaries, and proof command guidance, or `todo.md` explains why the
  umbrella must stay active for more classification.

### Step 4: Split and Switch Before Implementation

Goal: create lifecycle state for implementation only after classification.

Actions:

- When Step 3 identifies a focused semantic owner, create a new
  `ideas/open/*.md` file with goal, scope, out-of-scope items, acceptance
  criteria, and reviewer reject signals.
- Add a durable deactivation note to idea 295 summarizing the split decision,
  proof scope, and remaining parked buckets.
- Switch active lifecycle state from this umbrella to the focused owner before
  any implementation work begins.

Completion check:

- Active lifecycle state no longer points to idea 295 when implementation is
  ready to start, and idea 295 preserves the remaining inventory context.
