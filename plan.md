# Post-Wave Residual Baseline Failures Runbook

Status: Active
Source Idea: ideas/open/675_post_wave_residual_baseline_failures.md

## Purpose

Handle the remaining `test_baseline.new.log` failures after the 658 follow-up
wave without accepting a non-monotonic baseline candidate.

## Goal

Classify and repair or route the 9-failure post-wave candidate surface, with
priority on the two new-only RV64 CLI route failures.

## Core Rule

Do not accept `test_baseline.new.log` while it contains failures absent from
the accepted `test_baseline.log`. Compare by stable test name, not numeric row
id.

## Read First

- `ideas/open/675_post_wave_residual_baseline_failures.md`
- `test_baseline.log`
- `test_baseline.new.log`
- `log/baseline_5fef23bfa8b4eaf7f4cd2b897c25ff074d35209e.log`
- `docs/backend_baseline_history_triage/follow_up_order.md`
- Closure notes for ideas 668, 671, and 672

## Current Targets

- Candidate baseline: `test_baseline.new.log`, `9/3397` failed
- Accepted baseline: `test_baseline.log`, `11/3397` failed
- New-only candidate failures:
  - `backend_cli_failure_riscv64_pointer_global_local_publication_live_load_rejection`
  - `backend_cli_riscv64_call_arg_local_frame_address_materialization`
- Persistent failures:
  - destination/dump rows 92, 103, 109, 172
  - prepared-BIR AArch64 publication CLI row 322
  - LLVM torture rows 1941 and 1942

## Non-Goals

- Do not accept, rewrite, or delete baseline candidate evidence as proof of
  progress.
- Do not edit expectations, unsupported markers, allowlists, timeouts, runtime
  policy, or baseline accounting.
- Do not reopen closed ideas without fresh evidence contradicting their
  closure notes.
- Do not bundle multiple first owning layers into one implementation packet.

## Working Model

The candidate baseline improves total failure count from 11 to 9, but it is
not monotonic. Rows by stable test name show two candidate-only RV64 CLI route
failures, four accepted-only resolved failures, and seven persistent common
failures. The next repair route should start with the candidate-only failures
because accepting the candidate would otherwise bless new red rows.

## Execution Rules

- Preserve `test_baseline.new.log` for diagnosis until a fresh monotonic
  candidate exists.
- Record comparisons under `build/agent_state/675_*`.
- Use focused probes before implementation.
- For code-changing packets, run `cmake --build --preset default` plus the
  delegated focused proof before any broader baseline comparison.

## Steps

### Step 1: Capture Candidate Delta And First Owners

Goal: Produce a durable, name-based comparison between accepted and candidate
baseline failures.

Actions:

- Create `build/agent_state/675_step1_candidate_delta/summary.md`.
- Compare `test_baseline.log`, `test_baseline.new.log`, and the matching
  `log/baseline_5fef23bfa8b4eaf7f4cd2b897c25ff074d35209e.log` by stable test
  name.
- Record candidate-only, accepted-only, and common failures.
- For the two candidate-only failures, gather focused CLI/object route output
  and identify the first failing diagnostic or snippet boundary.

Completion Check:

- The two candidate-only rows have a first-owner hypothesis backed by fresh
  evidence, or the runbook names the exact missing probe needed.

### Step 2: Repair Or Split New-Only RV64 CLI Route Failures

Goal: Prevent the candidate baseline from normalizing newly exposed RV64 CLI
route failures.

Actions:

- If the two new-only rows share one proven owner, implement the smallest
  general repair.
- If they have different owners, create separate follow-up ideas and activate
  the first by dependency order.
- Preserve closed idea boundaries around 664, 673, and 674 unless fresh
  evidence proves their closure notes are wrong.

Completion Check:

- A focused proof shows the new-only rows are repaired, fail closed at a
  precise owner, or are split into separate active follow-up ideas.

### Step 3: Reconcile Persistent Common Failures

Goal: Decide which persistent rows remain actionable after the candidate-only
route is handled.

Actions:

- Re-read closure notes for ideas 668, 671, and 672.
- Classify rows 92, 103, 109, 172, 322, 1941, and 1942 as stale dump
  contract, active missing publication, research-only residual, or separate
  follow-up.
- Do not claim closure by expectation churn alone.

Completion Check:

- Every persistent common failure is either assigned to a focused follow-up or
  has a documented reason to remain intentionally deferred.

### Step 4: Refresh Baseline Candidate

Goal: Produce a fresh baseline candidate that can be accepted or rejected
cleanly.

Actions:

- Run the supervisor-delegated broad proof command for the selected scope.
- Compare the fresh candidate against the accepted baseline by stable test
  name.
- Accept only if monotonic under the repo baseline policy; otherwise reject
  and preserve diagnostic evidence.

Completion Check:

- Baseline state is either accepted as monotonic or rejected with a clear
  residual owner list.
