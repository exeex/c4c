# Backend Baseline History Umbrella Triage Runbook

Status: Active
Source Idea: ideas/open/658_backend_baseline_history_umbrella_triage.md

## Purpose

Classify the current noisy backend baseline using reverse-chronological log and
evidence history before choosing implementation repair work.

## Goal

Produce timestamp-grounded handoff docs and ordered follow-up ideas for the
current `log/baseline_*.log` failure surface.

## Core Rule

Do not implement fixes, weaken tests, accept baseline changes, or special-case
named failures under this umbrella. Use newest evidence first, then walk
backward by file timestamp when history conflicts.

## Read First

- `ideas/open/658_backend_baseline_history_umbrella_triage.md`
- `log/baseline_f3bf820c180dd4638ebd4db37e1223b759103665.log`
- `log/baseline_b5900d89fd30348c8e79901ce3e5e14875274d03.log`
- `log/baseline_575142db412ec5e846e77a6abcef8a77d7e97d83.log`
- `log/baseline_24c42f8da1ba7099134e97af222391de11ec5753.log`
- `log/baseline_11ee2da0f73d9328066ae05b84d75028df1adce7.log`
- `log/baseline_8a1ed1685aaa22a36153bc13c7acf08aeda05cd1.log`
- `log/baseline_9ae9ddfce0163d04b7df19e956175a6c8ccb1083.log`
- `log/baseline_0d937a44156ddf33c9df2691925d87e3f24a0978.log`
- `build/agent_state/657_step3_representative_pointer_value_store/summary.md`
- `build/agent_state/657_step4_representative_proof/summary.md`
- `build/agent_state/agent_logs/agents_codex_iter_21_844333.log`

## Current Targets

- Handoff directory: `docs/backend_baseline_history_triage/`
- Current broad evidence: newest `log/baseline_*.log`
- Regression history: `log/baseline_*.log` sorted by modification time
- Reverse-chronological evidence under `build/agent_state/`
- Open idea queue under `ideas/open/`

## Non-Goals

- Do not change implementation code.
- Do not edit tests, expectations, unsupported markers, allowlists, timeouts,
  runtime policy, or baseline acceptance state.
- Do not close or rewrite ideas 647, 655, or 657.
- Do not generate mixed-owner follow-up ideas.

## Working Model

The current baseline is too noisy for one direct implementation route. The
newest `log/baseline_*.log` reports 34 failures out of 3397 tests. The sorted
history shows a clean baseline on 2026-07-09 12:47, then 13 failures at 15:56,
20 at 17:57, 31 at 19:35, 32 at 22:53, and 34 from 2026-07-10 00:58 through
04:20. The failures span multiple layers, so the first repair must be selected
through ownership classification rather than named-row pressure. For
657-specific evidence, the newer 04:18 Step 3 representative summary
supersedes the older 04:12 Step 4 representative summary unless a new probe
proves otherwise.

## Execution Rules

- Create handoff docs before creating follow-up ideas.
- Record source file timestamps next to each evidence claim.
- Every failed baseline row must be assigned to exactly one provisional owner
  family or to an explicit unassigned bucket with a next probe.
- Follow-up ideas must name one owning layer and include concrete reviewer
  reject signals.
- If evidence conflicts, prefer the newest file and document the conflict.

## Steps

### Step 1: Build Reverse-Chronological Evidence Ledger

Goal: Establish which logs and summaries are authoritative for the current
baseline triage.

Actions:

- Create `docs/backend_baseline_history_triage/evidence_timeline.md`.
- List `log/baseline_*.log`, recent `build/agent_state/*` summaries, and the
  latest agent log in descending timestamp order.
- Record the current failed backend and LLVM torture rows from
  the newest `log/baseline_*.log`.
- Identify the failure-count change points in the sorted log history.
- Explicitly reconcile the older 657 Step 4 representative mismatch with the
  newer 657 Step 3 representative pass.

Completion Check:

- The ledger names the newest authoritative evidence for broad baseline state
  and representative 657 state.

### Step 2: Classify Failure Families By First Owning Layer

Goal: Turn the noisy failure list into repairable ownership families.

Actions:

- Create `docs/backend_baseline_history_triage/failure_classification.md`.
- Group every failed row from the newest `log/baseline_*.log` by first owning
  layer.
- Separate RV64 prepared destination/publication, pointer-local lowering,
  byval/prepared call-boundary, prepared object data/static storage,
  callee-saved GPR, packed local member, internal prepared BIR/CLI/AArch64,
  and LLVM torture rows unless evidence proves a tighter split.
- Mark rows as `assigned`, `blocked pending probe`, or `intentionally
  deferred`.

Completion Check:

- Every current failed row is accounted for exactly once, with the evidence
  path and timestamp that justifies the assignment.

### Step 3: Generate Ordered Follow-Up Ideas

Goal: Create implementation-ready source ideas for the highest-priority
ownership families.

Actions:

- Create `docs/backend_baseline_history_triage/follow_up_order.md`.
- Generate follow-up ideas under `ideas/open/` for the families required by
  the source idea unless Step 2 proves a smaller or better split.
- Give each follow-up idea one owning layer, clear in/out-of-scope boundaries,
  acceptance criteria, and concrete reviewer reject signals.
- Order follow-ups by evidence freshness, dependency, and breadth of affected
  baseline rows.

Completion Check:

- The follow-up order document and generated ideas agree on ordering and
  ownership boundaries.

### Step 4: Validate Umbrella Handoff And Prepare Lifecycle Close

Goal: Make the umbrella closure-ready without performing implementation work.

Actions:

- Verify the handoff docs agree on the same current evidence source.
- Verify generated follow-up ideas do not duplicate or silently absorb 647,
  655, or 657.
- Add a closure note to the source idea stating logs used, docs written,
  follow-up ideas generated, ordering rationale, and deferred rows.
- Leave implementation to the next activated follow-up idea.

Completion Check:

- The umbrella has produced durable docs and ordered follow-up ideas, and no
  implementation/test/baseline behavior changed.
