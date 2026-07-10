# Backend Baseline History Umbrella Triage Runbook

Status: Active
Source Idea: ideas/open/658_backend_baseline_history_umbrella_triage.md

## Purpose

Classify the current noisy backend baseline using reverse-chronological log and
evidence history before choosing implementation repair work.

## Goal

Produce timestamp-grounded handoff docs and ordered follow-up ideas for the
current `test_baseline.log` failure surface.

## Core Rule

Do not implement fixes, weaken tests, accept baseline changes, or special-case
named failures under this umbrella. Use newest evidence first, then walk
backward by file timestamp when history conflicts.

## Read First

- `ideas/open/658_backend_baseline_history_umbrella_triage.md`
- `test_baseline.log`
- `test_before.log`
- `build/agent_state/657_step3_representative_pointer_value_store/summary.md`
- `build/agent_state/657_step4_representative_proof/summary.md`
- `build/agent_state/agent_logs/agents_codex_iter_21_844333.log`

## Current Targets

- Handoff directory: `docs/backend_baseline_history_triage/`
- Current broad evidence: `test_baseline.log`
- Matching backend proof baseline: `test_before.log`
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
newest broad log reports 32 backend failures out of 368 backend tests plus two
LLVM torture rows. The backend failures span multiple layers, so the first
repair must be selected through ownership classification rather than named-row
pressure. For 657-specific evidence, the newer 04:18 Step 3 representative
summary supersedes the older 04:12 Step 4 representative summary unless a new
probe proves otherwise.

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
- List `test_baseline.log`, `test_before.log`, recent `build/agent_state/*`
  summaries, and the latest agent log in descending timestamp order.
- Record the current failed backend and LLVM torture rows from
  `test_baseline.log`.
- Explicitly reconcile the older 657 Step 4 representative mismatch with the
  newer 657 Step 3 representative pass.

Completion Check:

- The ledger names the newest authoritative evidence for broad baseline state
  and representative 657 state.

### Step 2: Classify Failure Families By First Owning Layer

Goal: Turn the noisy failure list into repairable ownership families.

Actions:

- Create `docs/backend_baseline_history_triage/failure_classification.md`.
- Group every failed row from `test_baseline.log` by first owning layer.
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
