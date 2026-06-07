# Plan: AArch64 Calls Preservation Republication Visibility Contract

Status: Active
Source Idea: ideas/open/121_aarch64_calls_preservation_republication_visibility_contract.md

## Purpose

Add route-visible evidence for calls preservation and republication lowering
before any owner extraction, fact movement, or shared-authority change is
attempted.

## Goal

Prove what prepared preservation facts AArch64 calls lowering consumes, which
side effects it publishes, and where stack-to-register republication happens.

## Core Rule

Do not move preservation ownership yet. This runbook proves visibility only:
prepared/shared code owns preservation route facts and endpoint data, while
AArch64 calls lowering owns target register spelling, frame-slot store lines,
and call-boundary machine records once those facts are known.

## Read First

- `ideas/open/121_aarch64_calls_preservation_republication_visibility_contract.md`
- `src/backend/mir/aarch64/codegen/calls.cpp`
- Existing prepared-printer, route-dump, and backend/AArch64 tests that mention
  call preservation, preserve-effect publication, prior stack-preserved
  arguments, or stack-to-register republication
- Closed idea 93 only if needed to verify stack-preserved source authority is
  not being reopened

## Current Targets

- Prepared preservation facts:
  - `PreparedCallPreservedValue`
  - prepared preservation routes
  - endpoint stack/register data
  - indexed first stack-preserved values
  - current prepared value homes
  - emitted-register state used by calls lowering
- Visibility surfaces:
  - prepared printer or route-dump tests, if an existing surface can expose the
    needed facts
  - backend/AArch64 instruction dispatch or call-boundary tests for
    preservation and republication paths
- AArch64 consumer/proof target:
  - `src/backend/mir/aarch64/codegen/calls.cpp`

## Non-Goals

- Do not extract a preservation owner before the visibility contract exists.
- Do not reopen stack-preserved source, endpoint stack storage, or frame-slot
  ownership from idea 93.
- Do not move AArch64 callee-saved register spelling, frame-slot store line
  spelling, preservation-home publication records, or ABI/local effect
  resources into shared code.
- Do not rework call preserve-effect semantics beyond narrow visibility needs.
- Do not perform broad calls lowering cleanup or line-count contraction.

## Working Model

Prepared/shared code owns preservation route facts and endpoint data.

AArch64 calls lowering consumes those facts and owns the target-local emission
details: callee-saved register spelling, frame-slot store lines, stack/register
republication records, and call-boundary machine records.

## Execution Rules

- Keep source idea edits unnecessary unless durable intent changes.
- Prefer adding visibility to existing dumps or focused tests before changing
  lowering code.
- If instrumentation is needed, keep it narrow and tied to route evidence.
- Treat owner extraction, authority migration, or helper renames as out of
  scope unless a later lifecycle plan explicitly authorizes them.
- Do not weaken tests, mark supported paths unsupported, or hide preservation
  side-effect changes behind expectation rewrites.
- For code-changing steps, run a fresh build before claiming readiness.
- Use focused backend/AArch64 and prepared-printer proof for visibility packets.
  Escalate if shared prepared preservation semantics change.

## Ordered Steps

### Step 1: Characterize Existing Preservation Visibility

Goal: map the current preservation and republication evidence before adding
new visibility.

Primary targets:

- existing prepared-printer or route-dump tests
- existing backend/AArch64 preservation and republication tests
- `src/backend/mir/aarch64/codegen/calls.cpp`

Actions:

- Inspect where `PreparedCallPreservedValue`, prepared preservation routes,
  endpoint stack/register data, indexed first stack-preserved values, current
  prepared value homes, and emitted-register state are consumed.
- Identify existing tests or dumps that already expose preserve-effect
  publication enabled and disabled.
- Identify existing tests or dumps that show prior stack-preserved argument
  consumption before a call.
- Identify existing tests or dumps that show stack-to-register republication
  through the same route.
- Record visibility gaps and the narrowest proof subset for later packets.

Completion check:

- `todo.md` records the cluster map, existing visibility surfaces, gaps for
  enabled/disabled preserve-effect publication, prior stack-preserved argument
  consumption, stack-to-register republication, and the recommended focused
  proof subset.

### Step 2: Add Preserve-Effect Publication Visibility

Goal: make preserve-effect publication state visible for both enabled and
disabled routes.

Primary targets:

- existing prepared-printer or route-dump visibility surface, if available
- focused backend/AArch64 tests if the route is only observable through target
  lowering

Actions:

- Add or update focused visibility so reviewers can distinguish
  preserve-effect publication enabled from disabled.
- Keep the visibility tied to prepared preservation facts rather than
  reselecting stack sources or endpoint homes.
- Avoid changing preservation semantics except where a missing observation hook
  requires a narrow diagnostic or dump addition.

Completion check:

- Focused proof shows enabled and disabled preserve-effect publication states,
  the build passes if code changed, and `todo.md` records the exact proof run.

### Step 3: Add Prior Stack-Preserved Argument Visibility

Goal: prove prior stack-preserved argument consumption before the call without
reopening stack-preserved source ownership.

Primary targets:

- focused backend/AArch64 preservation tests
- prepared route or dump tests, if they already model prior stack-preserved
  arguments
- `src/backend/mir/aarch64/codegen/calls.cpp` only as a consumer/proof target

Actions:

- Add or update route-visible coverage for prior stack-preserved argument
  consumption before a call.
- Show the prepared source or endpoint fact being consumed rather than
  recomputed locally.
- Keep AArch64 work limited to target-local emission and record construction.

Completion check:

- Focused proof shows prior stack-preserved argument consumption, the build
  passes if code changed, and `todo.md` records any remaining neighboring
  coverage gaps.

### Step 4: Add Stack-To-Register Republication Visibility

Goal: prove stack-to-register republication through the preservation route.

Primary targets:

- focused backend/AArch64 preservation or call-boundary tests
- prepared route or dump tests that expose preservation endpoint data

Actions:

- Add or update route-visible coverage for stack-to-register republication.
- Show how prepared preservation endpoint data drives the republication record.
- Keep callee-saved register spelling, frame-slot store line spelling, and
  machine-record emission target-local.

Completion check:

- Focused proof shows stack-to-register republication through prepared
  preservation facts, the build passes if code changed, and `todo.md` records
  the exact proof run.

### Step 5: Acceptance Review Package

Goal: decide whether idea 121 is complete under its source criteria.

Actions:

- Compare the final diff against the visibility contract and reviewer reject
  signals in the source idea.
- Confirm no preservation owner extraction, stack-source reselection, endpoint
  storage reopening, or shared target-specific movement was accepted as
  progress.
- Confirm route-visible proof covers preserve-effect publication enabled and
  disabled, prior stack-preserved argument consumption, and stack-to-register
  republication.
- Run the supervisor-selected close proof or prepare exact validation
  recommendations for the supervisor.

Completion check:

- `todo.md` records whether the source idea appears closure-ready, what proof
  was run, and any residual follow-up needed before lifecycle closure.
