# AArch64 C-Testsuite Failure Family Inventory

Status: Active
Source Idea: ideas/open/284_aarch64_c_testsuite_failure_family_inventory.md

## Purpose

Refresh the AArch64 backend c-testsuite failure inventory, identify the next
semantic failure family, and split that family into a focused repair idea
before implementation starts.

## Goal

Produce a current, bounded AArch64 backend failure-family inventory and switch
active lifecycle state to one focused semantic repair idea.

## Core Rule

This is an inventory and lifecycle-routing plan, not an implementation plan.
Do not claim progress by fixing one named testcase, changing expectations,
weakening CTest contracts, editing allowlists, altering unsupported
classifications, changing timeout policy, or expanding broad scans into
ordinary executor work without timeout and stale-process cleanup.

## Read First

- `ideas/open/284_aarch64_c_testsuite_failure_family_inventory.md`
- `todo.md`
- Recent closed AArch64 backend ideas under `ideas/closed/`, especially:
  - `ideas/closed/285_aarch64_backend_nonleaf_call_frame_lr_preservation.md`
  - `ideas/closed/286_aarch64_scalar_call_value_semantics.md`
  - `ideas/closed/287_aarch64_string_global_address_external_call_lowering.md`
  - `ideas/closed/288_aarch64_stack_frame_sp_alignment.md`
  - `ideas/closed/289_aarch64_function_pointer_indirect_call_values.md`
  - `ideas/closed/290_aarch64_scalar_parameter_alu_authority.md`
  - `ideas/closed/291_aarch64_call_argument_register_authority.md`
- Existing scan artifacts only if still present and current:
  - `/tmp/c4c_aarch64_backend_scan_212.log`
  - `/tmp/c4c_aarch64_backend_scan_212.classified`

## Current Targets

- AArch64 backend c-testsuite route under `build-aarch64-scan`.
- Remaining failing semantic families after the closed AArch64 repair ideas.
- Representative cases from each remaining bucket, not one-off filename fixes.
- Source-idea creation for the next focused semantic owner.

## Non-Goals

- Do not edit implementation files or tests under this umbrella plan.
- Do not ask an executor to implement a repair from this inventory idea.
- Do not treat timeout or hang cases as ordinary runtime failures without a
  timeout-safe command and stale generated-process cleanup.
- Do not weaken expected outputs, allowlists, unsupported classifications,
  CTest registration, runner behavior, or timeout settings.
- Do not merge frontend-owned failures with backend/runtime-owned failures.
- Do not reopen closed owners unless new proof contradicts their closure.

## Working Model

Previous inventory passes split and closed focused AArch64 backend owners for
non-leaf LR preservation, scalar call-value semantics, string/global external
call lowering, stack-frame SP alignment, function-pointer indirect-call values,
scalar parameter/ALU authority, and call-argument register authority. The next
inventory step should refresh the post-closure scan, classify what remains, and
create one new focused source idea for the strongest semantic owner.

## Execution Rules

- Prefer existing current scan artifacts only if they match the current build
  and closed-owner state.
- If rerunning the broad scan, use an explicit timeout and check for stale
  generated-runtime processes afterward.
- Record inventory results and owner-boundary evidence in `todo.md`.
- Create a new `ideas/open/*.md` only after a semantic family is clear enough
  to review independently.
- Switch lifecycle state to the focused idea before any implementation work.
- Keep timeout/hang work separated unless the inventory proves it is the next
  safest owner.

## Steps

### Step 1: Rehydrate Inventory Context

Goal: Establish the current post-closure baseline and determine whether a new
scan is required.

Primary target: source idea 284, recent closed AArch64 ideas, and available
scan artifacts.

Actions:

- Confirm no active `plan.md`/`todo.md` from another idea exists before this
  activation.
- Read the durable inventory findings and recent closed AArch64 owner notes.
- Check whether the existing `/tmp` scan artifacts are present, current, and
  sufficient for post-291 classification.
- Record in `todo.md` whether the next packet should reuse artifacts or rerun
  the safe broad scan.

Completion check: `todo.md` names the latest trusted scan source, known closed
owners, and the exact command needed if the inventory must be refreshed.

### Step 2: Refresh Safe AArch64 Backend Scan

Goal: Obtain a current failure inventory for the AArch64 backend c-testsuite
route.

Primary target: `build-aarch64-scan` AArch64 backend c-testsuite label.

Actions:

- Build `c4cll` for `build-aarch64-scan` if needed.
- Run the broad scan only with an explicit timeout, for example:
  `ctest --test-dir build-aarch64-scan -L '^aarch64_backend$' -j 8 --timeout 5 --output-on-failure`.
- Store transient broad-scan output outside root proof-log filenames unless the
  supervisor explicitly chooses canonical regression logs.
- Check for stale generated-runtime processes after the scan.
- Classify pass, frontend fail, runtime nonzero, runtime mismatch, timeout,
  backend fail, and signal families.

Completion check: `todo.md` contains a current classified inventory with the
scan command, artifact paths, counts, and stale-process cleanup result.

### Step 3: Sample Remaining Failure Families

Goal: Identify semantic owners from representative cases rather than filenames.

Primary target: representative failures from the refreshed inventory buckets.

Actions:

- Sample small representatives from each remaining backend/runtime family.
- Inspect generated assembly, diagnostics, and runtime output enough to name
  the semantic failure shape.
- Separate frontend-owned failures from backend/runtime-owned failures.
- Defer timeout/hang cases unless they are now minimal, safe, and clearly
  isolated.
- Compare candidates against closed owners to avoid reopening already-complete
  work without contradictory evidence.

Completion check: `todo.md` records candidate failure families, representative
cases, rejected owners, and the selected next semantic owner or the reason no
family is ready to split.

### Step 4: Create Focused Follow-On Idea

Goal: Convert the selected semantic family into a durable focused repair idea.

Primary target: a new `ideas/open/*.md` source idea.

Actions:

- Create one focused source idea with intent, why it exists, in-scope work,
  out-of-scope work, acceptance criteria, and concrete reviewer reject signals.
- Keep the new idea semantic and reviewable; do not encode one testcase or one
  emitted instruction as the repair contract.
- Record any deferred timeout, frontend, aggregate, libc, or environment
  blockers in `todo.md` or the new idea only where they define scope.
- Do not edit implementation files or tests.

Completion check: one focused `ideas/open/*.md` exists for the selected family,
and `todo.md` explains why it is the next appropriate active repair idea.

### Step 5: Deactivate Inventory And Switch

Goal: Stop using the umbrella inventory as the active implementation target and
activate the focused repair idea.

Primary target: lifecycle files `plan.md`, `todo.md`, and the selected focused
source idea.

Actions:

- Add a compact deactivation note to source idea 284 preserving only durable
  inventory findings and the selected follow-on path.
- Remove or replace this inventory runbook.
- Activate the focused idea into a new `plan.md` and reset `todo.md` to that
  focused plan.
- Leave no implementation work attached to the umbrella inventory idea.

Completion check: active lifecycle state points to the focused follow-on idea,
not source idea 284, and the inventory findings needed to justify the switch
are preserved.
