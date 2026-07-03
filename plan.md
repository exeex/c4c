# RV64 Runtime And No-Diagnostic Failure Triage Runbook

Status: Active
Source Idea: ideas/open/549_rv64_runtime_and_no_diagnostic_triage.md

## Purpose

Reconstruct first-owner evidence for current RV64 failures that lack explicit
ownership before any implementation work is justified.

Goal: classify representative no-diagnostic compile failures, other
non-unsupported failures, and segmentation-fault exits into durable owner
families.

Core Rule: do not claim implementation progress from crash, timeout, or
compile-fail counts until a reproduced first bad fact identifies the owner.

## Read First

- ideas/open/549_rv64_runtime_and_no_diagnostic_triage.md
- Current bucket map or scan artifact that reported:
  - 503 compile failures without explicit `unsupported_*` diagnostic
  - 57 other non-unsupported failures
  - 7 segmentation-fault exits
- Existing RV64 gcc_torture runner and log conventions
- Existing unsupported diagnostic and bucket classification helpers

## Current Scope

- Reproduce representative rows from each no-diagnostic family.
- Classify first ownership as BIR producer, prepared contract, RV64 lowering,
  runtime mismatch, test infrastructure, timeout, or unsupported feature.
- Preserve minimal commands and log locations for reviewer audit.
- Create follow-up source ideas only after evidence proves a high-frequency
  owner family.

## Non-Goals

- Do not implement crash or runtime fixes in this runbook.
- Do not weaken runtime comparison, expected output checks, unsupported
  markers, or pass/fail accounting.
- Do not count primary-F128 crashes as ordinary-C blockers.
- Do not merge runtime mismatch, producer repair, and RV64 lowering into one
  implementation route.
- Do not use named-case fixes as evidence of broad capability progress.

## Working Model

Treat the 567 rows as evidence gaps. The first useful output is an auditable
owner map, not a passing testcase. Representative reproduction should narrow a
family until a future implementation idea can state a precise producer,
contract, lowering, runtime, or infrastructure boundary.

## Execution Rules

- Keep routine packet notes and temporary commands in `todo.md`.
- Preserve source intent in the idea file unless activation or later lifecycle
  review proves the durable idea itself changed.
- Prefer semantic first-owner evidence over testcase-shape matching.
- If a reproduced family is mainly F128, route it to the F128 quarantine lane
  instead of ordinary-C repair.
- If unrelated owner families appear, split them into separate open ideas
  rather than expanding this runbook into implementation work.
- For any code-changing follow-up created later, require build proof plus the
  narrow representative test subset named in that follow-up.

## Ordered Steps

### Step 1: Reconstruct Representative No-Diagnostic Families

Goal: identify representative rows and commands for each current no-diagnostic
failure group.

Primary target: current RV64 gcc_torture bucket map, runner output, and raw
logs.

Actions:

- Locate the freshest bucket map or scan artifact behind the 503/57/7 counts.
- Select a small representative sample from each no-diagnostic family,
  preserving source filename, command, exit mode, and available stderr/stdout.
- Re-run representatives without changing expectations or unsupported
  markers.
- Record reproduction commands and log paths in `todo.md`.

Completion check:

- Each sampled family has a current reproduction command and either a log path
  or a clear note explaining why reproduction is blocked.
- No implementation files, expectations, or unsupported classifications were
  changed.

### Step 2: Classify First Owners

Goal: assign each representative to the earliest proven owner boundary.

Primary target: BIR producer diagnostics, prepared contract checks, RV64
lowering paths, runtime output comparison, and test infrastructure.

Actions:

- Inspect each reproduced representative for the first bad fact.
- Classify rows as BIR producer, prepared contract, RV64 lowering, runtime
  mismatch, test infrastructure, timeout, unsupported feature, or still
  unclassified evidence gap.
- Separate primary-F128 cases into quarantine notes instead of ordinary-C
  blocker lists.
- Keep runtime mismatches separate from compile-time producer or lowering
  failures.

Completion check:

- Every representative has a first-owner classification or an explicit
  evidence-gap note.
- Classification evidence names the command/log fact that supports it.
- No runtime comparison or expected-output behavior was weakened.

### Step 3: Split Durable Follow-Up Ideas

Goal: turn proven high-frequency owner families into separate implementation
or investigation ideas without broadening this active runbook.

Primary target: `ideas/open/` only for new durable follow-up source ideas.

Actions:

- Group representatives by proven owner and likely shared implementation
  boundary.
- Create a new open idea only when a family has enough evidence to support a
  narrow owner, scope, acceptance criteria, and reviewer reject signals.
- Keep runtime mismatch, BIR producer, prepared contract, RV64 lowering,
  timeout, infrastructure, and F128 quarantine work separate.
- Leave low-confidence families as evidence gaps in `todo.md` rather than
  inventing implementation scope.

Completion check:

- High-confidence high-frequency owner families have separate durable follow-up
  ideas.
- Any created idea includes concrete reviewer reject signals against
  testcase-shaped shortcuts and expectation downgrades.
- Unsplit families have an explicit reason they remain evidence gaps.

### Step 4: Consolidate Triage Evidence For Review

Goal: make the triage result easy for the supervisor and reviewer to audit.

Primary target: `todo.md` packet summary and referenced reproduction logs.

Actions:

- Summarize reproduced samples, classifications, F128 quarantine findings, and
  newly created follow-up ideas.
- Identify any rows or families still blocked by missing logs, stale scan
  artifacts, or runner failures.
- Recommend the next lifecycle decision: execute a follow-up idea, run another
  evidence pass, or close this triage idea if acceptance criteria are met.

Completion check:

- `todo.md` contains the latest packet summary, suggested next action,
  watchouts, and proof commands/logs.
- The source idea's acceptance criteria can be evaluated from the recorded
  evidence.
- The supervisor can choose the next active idea without re-deriving the
  triage route.
