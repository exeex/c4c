# Route 6 Call Source Consumer Runbook

Status: Active
Source Idea: ideas/open/213_route6_call_source_consumer.md

## Purpose

Activate one narrow Route 6 follow-up that moves exactly one call argument or
result source consumer to route/prepared agreement while preserving prepared
call-plan authority.

## Goal

Select one named call argument or result source consumer and make Route 6
evidence supply the same source only when it agrees with prepared call-plan
semantics, with prepared fallback for absent, invalid, duplicate/conflict, and
mismatched Route 6 facts.

## Core Rule

Route 6 may identify the selected call-use source only. Prepared call plans
remain authoritative for ABI placement, layout, helper protocols, storage,
movement, aggregate transport, final call records, wrappers, printer output,
and emitted behavior.

## Read First

- `ideas/open/213_route6_call_source_consumer.md`
- `docs/bir_prealloc_fusion/phase_d2_retained_surface_consumer_switch_analysis.md`
- Route 6 call-use source lookup and diagnostic code
- Prepared call-plan, call printer, and x86 route-debug tests

## Current Targets

- One named call argument or result source consumer.
- Route/prepared agreement for the selected source.
- Prepared fallback for no Route 6 fact, invalid call-use references,
  duplicate/conflicting facts, and route/prepared mismatch.
- Byte-stable prepared call printer and x86 route-debug output.

## Non-Goals

- Do not migrate ABI placement, wrapper kind, clobbers, outgoing stack sizing,
  byval lanes, variadic FPR counts, helper/carrier protocols, value homes,
  move bundles, publication routing, aggregate transport, final call records,
  storage, movement, or emitted output.
- Do not perform broad x86 call wrapper migration, remove public fallback, or
  delete call-plan APIs.
- Do not rewrite expectations, refresh baselines, rename helpers, downgrade
  supported paths, or weaken test contracts as proof of progress.
- Do not claim Route 6 call-plan ownership from one selected source consumer.

## Working Model

- The selected consumer is a semantic proof surface, not a new owner for call
  ABI, layout, helper, storage, movement, wrapper, or final-call policy.
- Route 6 evidence is usable only when it identifies the same source that the
  prepared call plan would use for the selected consumer.
- Missing, invalid, duplicate/conflicting, and mismatched Route 6 facts must
  preserve existing prepared source behavior.
- Prepared call printer and x86 route-debug output spelling remain
  compatibility surfaces.

## Execution Rules

- Keep every implementation packet tied to one named call argument or result
  source consumer.
- Record the selected consumer, prepared baseline behavior, fallback
  dimensions, and narrow proof command in `todo.md` before changing
  implementation files.
- Add Route 6 evidence behind prepared fallback; do not bypass prepared
  call-plan ownership.
- Treat expectation rewrites, unsupported downgrades, helper renames,
  baseline refreshes, and named-case shortcuts as non-progress.
- For code-changing packets, run fresh build or compile proof plus the narrow
  Route 6 call-source test subset selected by the supervisor.
- Escalate to x86 route-debug, call-printer, or broader backend validation if
  shared call-plan helpers, output strings, wrappers, or final-call records are
  touched.

## Steps

### Step 1: Name the Consumer and Baseline Prepared Behavior

Goal: identify the exact call argument or result source consumer and record
its current prepared behavior before implementation changes.

Primary targets:
- Route 6 call-use source lookup paths
- Prepared call-plan consumers for call arguments or results
- Prepared call printer and x86 route-debug expected-string tests

Actions:
- Inspect call argument and result source consumers and select one exact
  consumer to migrate.
- Confirm the selected consumer currently obtains its source from prepared
  call-plan data.
- Record the selected consumer, prepared baseline behavior, fallback
  dimensions, current coverage, missing proof cases, and narrow proof command
  in `todo.md`.
- Identify existing positive, absent, invalid, duplicate/conflict, mismatch,
  printer/debug, wrapper, ABI, aggregate transport, and final-call coverage for
  that consumer.

Completion check:
- `todo.md` names one selected consumer, proof command, existing coverage, and
  missing proof cases without implementation changes.

### Step 2: Add Route-Native Evidence for the Consumer

Goal: let Route 6 provide the selected call-use source only when it agrees
with prepared call-plan semantics.

Primary targets:
- The selected call argument or result source consumer
- Route 6 call-use source lookup helpers
- Prepared fallback paths used by the selected consumer

Actions:
- Reuse or add a narrow helper that reads Route 6 evidence for the selected
  consumer.
- Require agreement with the prepared call-use identity, source value, result
  or argument role, call site, and any consumer-specific source fields.
- Preserve prepared behavior for no Route 6 fact, invalid call-use reference,
  duplicate/conflicting facts, and route/prepared mismatch.
- Keep ABI placement, wrapper kind, aggregate transport, final call records,
  storage, movement, helper protocols, and emitted output prepared or
  target-owned.

Completion check:
- The selected consumer can use Route 6 evidence under agreement, and every
  failed agreement path returns the existing prepared source behavior.

### Step 3: Prove Fail-Closed Call-Use Diagnostics

Goal: make the positive and negative Route 6 contract observable for the
selected consumer.

Primary targets:
- Tests for the selected call-use source consumer
- Tests for absent, invalid, duplicate/conflict, and mismatch cases
- Tests proving adjacent call-plan policy remains prepared-owned

Actions:
- Cover route/prepared agreement for the selected consumer.
- Cover no Route 6 fact and absent evidence.
- Cover invalid call-use references when the selected path can observe them.
- Cover duplicate or conflicting Route 6 facts when applicable.
- Cover route/prepared disagreement and prove the prepared source is retained.
- Prove ABI, wrapper kind, aggregate transport, and final call records remain
  prepared or target-owned.

Completion check:
- Narrow tests prove positive, absent, invalid, duplicate/conflict when
  applicable, mismatch, and adjacent prepared-owned call-plan behavior without
  weakening expected strings or supported-path contracts.

### Step 4: Preserve Printer and x86 Route-Debug Strings

Goal: prove the consumer migration did not alter byte-stable call printer,
wrapper, or x86 route-debug surfaces.

Primary targets:
- Prepared call printer expected-string tests
- x86 route-debug expected-string tests
- Wrapper or final-call tests selected by the supervisor when relevant

Actions:
- Run the selected prepared call printer proof for the named consumer.
- Run x86 route-debug no-change proof for the selected source surface.
- Run wrapper or final-call stability checks if shared call-plan helpers,
  target wrappers, or output surfaces were touched.
- Do not update expected strings unless a separate approved semantic change
  requires it.

Completion check:
- Proof logs show prepared call printer and x86 route-debug output remain
  byte-stable, with wrapper and final-call no-change coverage where
  applicable.

### Step 5: Acceptance Review

Goal: prepare the slice for supervisor review without expanding Route 6
ownership.

Actions:
- Compare the final diff against the source idea reviewer reject signals.
- Verify no broad call-plan, ABI, wrapper, final-call, storage, movement, or
  x86 migration slipped in.
- Verify route/prepared agreement is semantic and fail-closed, not
  testcase-shaped call-use handling.
- Keep routine progress and proof notes in `todo.md`; do not edit the source
  idea unless durable intent changes.

Completion check:
- The slice satisfies the source idea acceptance criteria and is ready for
  supervisor-side validation and commit.
