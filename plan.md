# AArch64 Publication Ordering Evidence Probe Runbook

Status: Active
Source Idea: ideas/open/76_aarch64_publication_ordering_evidence_probe.md

## Purpose

Trace representative publication paths from prepared facts to AArch64
publication records before deciding whether ordering is already shared
authority or needs a new prepared publication-order query.

## Goal

Classify publication materialization ordering for edge-copy, call-boundary,
and typed stack-source paths, then choose a bounded no-code conclusion or
implementation route.

## Core Rule

Do not add local ordering helpers, migrate publication lowering, or rewrite
publication expectations until the probe proves where ordering authority lives.

## Read First

- `ideas/open/76_aarch64_publication_ordering_evidence_probe.md`
- `src/backend/mir/aarch64/codegen/dispatch_publication.cpp`
- `src/backend/mir/aarch64/codegen/calls.cpp`
- `src/backend/mir/aarch64/codegen/memory.cpp`
- Prepared publication lookup surfaces that already feed AArch64 records.

## Current Scope

- Trace one edge-copy publication from prepared source facts to AArch64
  publication records.
- Trace one call-boundary publication from prepared call/publication facts to
  AArch64 records.
- Trace one typed stack-source publication from prepared stack-source facts to
  AArch64 records.
- Decide whether existing prepared publication facts already carry ordering or
  whether a new prepared publication-order query is required.

## Non-Goals

- Do not implement publication cleanup before the ordering authority is known.
- Do not reopen the whole dispatch-family contraction.
- Do not bundle broad call, memory, or dispatch cleanup into the evidence pass.
- Do not weaken publication expectations or supported behavior as proof.
- Do not add named-testcase or path-shaped publication ordering shortcuts.

## Working Model

- Publication record construction may remain target-local, but ordering
  decisions should not be re-derived separately in dispatch, call, and memory
  paths if prepared publication facts already own them.
- The probe must distinguish existing prepared authority, target-local record
  spelling, and genuinely missing shared ordering facts.
- Implementation is optional and only follows if the trace proves a concrete
  missing or duplicated ordering authority.

## Execution Rules

- Record probe findings in `todo.md` before code changes.
- Keep the source idea stable unless durable intent changes.
- Keep packets narrow: trace first, contract second, implementation only if
  required.
- Every code-changing step needs fresh build proof plus focused publication
  ordering tests for the affected paths.
- Use canonical regression logs before accepting any code slice.

## Step 1: Trace Edge-Copy Publication Ordering

Goal: determine whether edge-copy publication ordering is already supplied by
prepared facts or locally re-derived by AArch64 dispatch publication lowering.

Primary targets:
- `src/backend/mir/aarch64/codegen/dispatch_publication.cpp`
- Prepared edge-copy or publication source lookup surfaces consumed by
  dispatch publication lowering.

Actions:
- Follow one representative edge-copy publication from prepared source facts to
  the AArch64 publication record.
- Identify where publication order, source binding, destination binding, and
  record emission order are chosen.
- Classify each ordering decision as already prepared/shared, target-local by
  design, or missing shared authority.
- Record the trace in `todo.md` with exact owners and function/helper names.

Completion check:
- `todo.md` contains the edge-copy trace and a clear classification of whether
  ordering is prepared, target-local, or missing for that path.
- No implementation files have changed for this evidence-only step.

## Step 2: Trace Call-Boundary Publication Ordering

Goal: determine whether call-boundary publication ordering is already supplied
by prepared call/publication facts or locally re-derived in AArch64 call
lowering.

Primary targets:
- `src/backend/mir/aarch64/codegen/calls.cpp`
- Prepared call/publication lookup surfaces consumed by AArch64 calls.

Actions:
- Follow one representative call-boundary publication from prepared facts to
  the AArch64 publication record.
- Identify where argument, result, clobber, or call-edge publication order is
  chosen when relevant.
- Classify each ordering decision as already prepared/shared, target-local by
  design, or missing shared authority.
- Record the trace in `todo.md` with exact owners and function/helper names.

Completion check:
- `todo.md` contains the call-boundary trace and a clear classification of
  whether ordering is prepared, target-local, or missing for that path.
- Any relationship to the Step 1 edge-copy ordering model is explicit.

## Step 3: Trace Typed Stack-Source Publication Ordering

Goal: determine whether typed stack-source publication ordering is already
supplied by prepared stack-source facts or locally re-derived in AArch64 memory
lowering.

Primary targets:
- `src/backend/mir/aarch64/codegen/memory.cpp`
- Prepared stack-source and publication lookup surfaces consumed by AArch64
  memory lowering.

Actions:
- Follow one representative typed stack-source publication from prepared facts
  to the AArch64 publication record.
- Identify where stack source, type, publication order, and record emission
  order are chosen.
- Classify each ordering decision as already prepared/shared, target-local by
  design, or missing shared authority.
- Record the trace in `todo.md` with exact owners and function/helper names.

Completion check:
- `todo.md` contains the typed stack-source trace and a clear classification
  of whether ordering is prepared, target-local, or missing for that path.
- The three required publication paths can be compared without further
  archaeology.

## Step 4: Decide Publication-Order Authority

Goal: decide whether publication ordering can remain on existing prepared
publication facts or needs a new prepared publication-order query.

Actions:
- Compare the three traces from Steps 1 through 3.
- Name the existing prepared fact or query that owns ordering if one is
  sufficient.
- If ordering is missing or repeatedly re-derived, draft the proposed prepared
  publication-order query shape, owner, consumers, and target-local
  responsibilities.
- If no implementation is required, record the no-code conclusion and prepare
  the lifecycle state for review or closure.

Completion check:
- `todo.md` or a plan checkpoint states one concrete route: consume existing
  prepared authority, implement a new prepared publication-order query, or
  close with a no-code evidence conclusion.
- Reviewer reject signals from the source idea are still enforceable from the
  recorded evidence.

## Step 5: Implement Ordering Authority If Required

Goal: repair only the ordering duplication or missing query proven by Step 4.

Actions:
- Add or extend the prepared publication lookup surface named in Step 4.
- Migrate only the AArch64 consumers that Step 4 proved are re-deriving
  ordering.
- Keep target-local publication record construction and final AArch64 spelling
  in AArch64 owners.
- Avoid broad dispatch, call, or memory rewrites outside the named ordering
  contract.

Completion check:
- Focused tests cover edge-copy, call-boundary, and typed stack-source
  publication ordering for the affected route.
- Build proof is fresh.
- Regression guard accepts matching before/after logs for the chosen scope.

## Step 6: Acceptance Review And Closure Decision

Goal: decide whether idea 76 is complete or whether a separate follow-up idea
is required.

Actions:
- Compare the final evidence and any implementation against the source idea.
- Reject local ordering helper additions that bypass prepared facts,
  named-case shortcuts, expectation downgrades, or bundled broad rewrites.
- Run or consume broader regression guard coverage appropriate to any touched
  implementation surface.
- If a separate initiative remains, create it under `ideas/open/` instead of
  expanding this plan silently.

Completion check:
- A reviewer can see whether publication ordering authority already existed,
  was added, or was deferred into a separate source idea.
- The active lifecycle state is ready for close, deactivation, or plan rewrite.
