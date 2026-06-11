# Full-Suite Baseline String-Authority and Timeout Attribution Runbook

Status: Active
Source Idea: ideas/open/199_full_suite_baseline_string_authority_timeout_attribution.md

## Purpose

Identify where the accepted 3428/3428 full-suite baseline drifted to the
rejected 3426/3428 candidate, and classify the two known failures before any
repair or refreshed baseline is accepted.

## Goal

Produce first-bad evidence for `string_authority_guard` and independently
classify the `c_testsuite_aarch64_backend_src_00040_c` timeout as deterministic
or ambient timeout/noise.

## Core Rule

Do not repair, weaken expectations, disable guards, or accept
`test_baseline.new.log` until the chronological failure family and first bad
step are understood.

## Read First

- `ideas/open/199_full_suite_baseline_string_authority_timeout_attribution.md`
- `test_baseline.log`, if present.
- `test_baseline.new.log`, if present.
- `test_before.log`, if present.
- `test_after.log`, if present.
- Closed ideas 191-198.
- Durable docs and lifecycle artifacts produced by ideas 191-198.
- Workflow and guard references for `string_authority_guard`.

## Scope

- Chronological review of baseline and proof logs.
- Correlation of accepted and rejected full-suite candidates with commits from
  the ideas 191-198 sequence.
- Narrow reproduction of `string_authority_guard`.
- Separate reproduction/classification of
  `c_testsuite_aarch64_backend_src_00040_c`.
- Manual first-bad isolation across the Phase D follow-up readiness and
  lifecycle naming sequence if the guard failure reproduces.
- A focused repair route only after first-bad evidence exists.

## Non-Goals

- Do not accept `test_baseline.new.log` while it is regressive against the
  accepted 3428/3428 baseline.
- Do not treat the `00040` timeout and `string_authority_guard` as the same
  root cause without separate reproduction evidence.
- Do not weaken, disable, or bypass `string_authority_guard`.
- Do not delete, rename, or broadly rewrite Phase D or Phase E docs just to
  satisfy the guard.
- Do not open draft 155 or proceed with true Phase E retirement while the
  baseline candidate is regressive.
- Do not repair unrelated failures unless first-bad evidence ties them to this
  failure family.

## Working Model

The rejected candidate is associated with commit
`1d1c506f05dbaf783f6293d61e4b05d339395c4e` and reports 3426/3428 passing, with
known failures in `string_authority_guard` and
`c_testsuite_aarch64_backend_src_00040_c`. The candidate appears in a
docs/lifecycle-heavy sequence, so the route must consider both ambient timeout
behavior and deterministic string-authority violations introduced by
readiness, naming, or generated lifecycle artifacts.

## Execution Rules

- Start with analysis and reproduction only.
- Preserve the accepted 3428/3428 baseline as the authority until a
  non-regressive full-suite candidate is proven.
- Review logs by filesystem timestamp and commit order; do not infer first-bad
  from the rejected candidate subject alone.
- Keep string-authority failures separate from timeout classification until
  evidence proves they share a root cause.
- If a deterministic violation is found, identify the authority rule and the
  offending artifact before proposing repair.
- If the correct fix is a separate initiative, open or request a focused repair
  idea with first-bad evidence instead of expanding this plan ad hoc.
- For any code or docs repair delegated after attribution, prove the implicated
  narrow guard first, then any affected workflow/doc checks, then a
  non-regressive full-suite baseline candidate.

## Steps

### Step 1: Build the Baseline and Commit Timeline

Goal: establish the last accepted 100% baseline, the rejected 99% candidate,
and the ordered lifecycle/docs sequence between them.

Primary targets:
- `test_baseline.log`
- `test_baseline.new.log`
- `test_before.log`
- `test_after.log`
- closed ideas 191-198
- durable docs artifacts from ideas 191-198
- `todo.md` history around rejected baseline candidates

Actions:
- List baseline/proof logs by filesystem timestamp.
- Inspect the accepted and rejected baseline summaries.
- Inspect the ordered commits around the Phase D follow-up readiness and
  lifecycle naming sequence.
- Search docs, closed ideas, review artifacts, and lifecycle notes for
  `string_authority_guard`, `test_baseline.new`, `3426/3428`, `00040`, `99%`,
  `baseline candidate`, and `string authority`.
- Record which commits and artifacts are plausible first-bad candidates.

Completion check:
- `todo.md` records the baseline timeline, candidate commit range, known log
  evidence, and any missing artifacts.

### Step 2: Reproduce and Classify the Two Failures Separately

Goal: determine whether each known failure is deterministic, intermittent, or
not reproducible in the current checkout.

Actions:
- Run the narrow `string_authority_guard` command and record whether it
  deterministically fails.
- Run `c_testsuite_aarch64_backend_src_00040_c` separately enough to classify
  timeout/noise versus deterministic backend behavior.
- Keep proof output in the canonical log path delegated by the supervisor.
- Do not change source, docs, tests, expectations, or baseline logs during this
  step unless explicitly delegated.

Completion check:
- `todo.md` records independent reproduction results for
  `string_authority_guard` and `00040`, including commands and outcomes.

### Step 3: Isolate the First Bad String-Authority Step

Goal: if the guard reproduces, identify the exact commit or lifecycle slice
where it first appears.

Actions:
- Bisect manually across the ordered Phase D follow-up readiness and lifecycle
  naming commits using the narrow guard command.
- Inspect candidate diffs for docs, lifecycle, generated route/readiness text,
  or naming artifacts that introduce the guarded string pattern.
- Check whether historical baseline logs under `log/` show the same guard
  before the Phase D follow-up sequence.
- Keep the timeout classification from Step 2 separate from this guard-focused
  first-bad investigation.

Completion check:
- `todo.md` names the first bad commit or records why the available evidence
  cannot identify one yet.

### Step 4: Decide the Repair or Follow-Up Route

Goal: translate attribution evidence into the smallest acceptable next action.

Actions:
- If the guard failure is deterministic, explain the string-authority or
  lifecycle naming rule that was violated.
- Determine whether the offending artifact should be repaired under this
  active idea or whether a focused repair idea is required.
- If the `00040` timeout is ambient or non-deterministic, document it separately
  from the string-authority route.
- If the `00040` behavior is deterministic and unrelated, route it as a
  separate initiative instead of merging it into the guard repair.
- Preserve Phase D versus true Phase E distinctions in any docs or lifecycle
  change.

Completion check:
- `todo.md` records the chosen route and why it satisfies the source idea
  acceptance criteria without overfitting the rejected baseline candidate.

### Step 5: Prove the Final Baseline Decision

Goal: accept only a non-regressive final state, or leave a focused repair idea
with first-bad evidence attached.

Actions:
- After any accepted repair, run the narrow guard, the separated timeout
  reproducer if needed, and any implicated workflow or docs tests.
- Run a full-suite baseline candidate before replacing or accepting the
  baseline authority.
- Accept a refreshed `test_baseline.log` only if it is non-regressive relative
  to the accepted 3428/3428 baseline.
- If no repair is made in this runbook, ensure the focused follow-up includes
  first-bad evidence and explicit reject signals.

Completion check:
- The final state either restores a 100% full-suite baseline candidate or
  leaves a focused repair idea with first-bad evidence; `todo.md` records final
  proof and whether the source idea is ready for plan-owner close.
