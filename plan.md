# RV64 gcc_torture 1000-Pass Recovery Umbrella Runbook

Status: Active
Source Idea: ideas/open/601_rv64_gcc_torture_1000_pass_recovery_umbrella.md

## Purpose

Turn the fresh RV64 gcc_torture backend-object scan into an evidence-backed
follow-up queue for recovering from `470/1467` passing cases toward `1000+`.

Goal: classify the current `997` failures by first owner and capability
family, write the required handoff docs, and generate ordered follow-up ideas
without implementing compiler, backend, harness, or expectation changes.

## Core Rule

This umbrella is triage and follow-up generation only. Do not edit
implementation files, test expectations, unsupported markers, allowlists,
runtime behavior, timeout policy, pass/fail accounting, or default harness
behavior.

## Read First

- Source idea:
  `ideas/open/601_rv64_gcc_torture_1000_pass_recovery_umbrella.md`
- Current scan command:
  `scripts/check_progress_rv64_gcc_c_torture_backend.sh`
- Current scan summary:
  `build/agent_state/rv64_gcc_c_torture_backend_summary.tsv`
- Current failed list:
  `build/agent_state/rv64_gcc_c_torture_backend_failed.txt`
- Per-case logs:
  `build/rv64_gcc_c_torture_backend/`
- Historical comparison baseline:
  `ideas/closed/420_rv64_gcc_torture_post_contract_umbrella.md`
- Recent architecture context:
  ideas `587` through `600` under `ideas/closed/`

## Current Targets

- `docs/rv64_gcc_torture_1000_pass_recovery/index.md`
- `docs/rv64_gcc_torture_1000_pass_recovery/current_scan_summary.md`
- `docs/rv64_gcc_torture_1000_pass_recovery/failure_bucket_map.md`
- `docs/rv64_gcc_torture_1000_pass_recovery/high_yield_followup_plan.md`
- `docs/rv64_gcc_torture_1000_pass_recovery/dependency_order_to_1000.md`
- New ordered follow-up ideas under `ideas/open/`

## Non-Goals

- Do not repair RV64, BIR, prepared/prealloc, ABI, runtime, harness, or test
  behavior inside this umbrella.
- Do not claim progress by changing expectations, unsupported status, timeout
  policy, allowlist membership, or accounting.
- Do not open mixed-owner implementation ideas that combine producer repair,
  prepared authority, RV64/MIR consumption, runtime mismatch, and test
  infrastructure in one route.
- Do not use stale `349/1467`, `404/1063`, or other historical counts as
  current evidence.
- Do not choose follow-ups because a testcase name is familiar from recent
  work.

## Working Model

- Current authoritative evidence is `1467` total, `470` passed, `997` failed,
  `0` missing from `scripts/check_progress_rv64_gcc_c_torture_backend.sh`.
- The older `349/1467` baseline is historical only and should be used only to
  explain broad movement since the previous umbrella.
- Bucket failures by first owning layer before ranking fixes.
- Prefer families that reuse or slightly extend the recent ideas `587` through
  `600` authority/freshness architecture.
- If a high-count bucket exposes unsettled architecture, create a research or
  discussion idea instead of forcing implementation.

## Execution Rules

- Keep every generated follow-up idea single-owner where possible.
- Every generated follow-up idea must include owning layer, prerequisites,
  estimated evidence breadth, proof surface, acceptance criteria, and reviewer
  reject signals.
- Rank by expected broad pass-count impact, dependency order, and semantic
  leverage, not by one named case.
- Explicitly quarantine or defer low-yield or policy-heavy families such as
  F128, string/library calls, builtins, and environment-dependent failures
  when they distract from the first `1000+` route.
- Use row-level evidence from summaries, failed lists, logs, diagnostics, and
  source case families. Do not stop at aggregate counts.
- Treat testcase-overfit as route drift.

## Ordered Steps

### Step 1: Establish the Evidence Baseline

Goal: confirm and document the scan evidence that all later classification
uses.

Primary target:
`docs/rv64_gcc_torture_1000_pass_recovery/current_scan_summary.md`

Actions:

- Inspect the current summary, failed list, and representative per-case logs.
- Record the scan command, paths, total/pass/fail/missing counts, and date or
  freshness context available from the files.
- Compare against the closed `420` umbrella only as historical context.
- Note whether any newer scan supersedes the `470/1467` result.

Completion check:

- `current_scan_summary.md` names `1467` total, `470` passed, `997` failed,
  `0` missing, and clearly marks `349/1467` as historical.

### Step 2: Build the Failure Bucket Map

Goal: classify the `997` failures by first owner and capability family.

Primary target:
`docs/rv64_gcc_torture_1000_pass_recovery/failure_bucket_map.md`

Actions:

- Group failures using diagnostics, generated object/runtime status, source
  case families, and representative logs.
- Separate BIR semantic producer gaps, prepared/prealloc authority gaps,
  RV64/MIR consumer gaps, runtime mismatch families, unsupported instruction
  fragments, ABI/aggregate/variadic/layout families, and policy-heavy
  low-priority lanes.
- Identify buckets that look close to repair because ideas `587` through `600`
  already supplied most of the authority/freshness contract.
- Identify architecture weak points that need discussion or research before
  implementation.

Completion check:

- `failure_bucket_map.md` contains first-owner plus capability-family buckets,
  not only raw counts, and accounts for the current failed population at the
  useful classification granularity.

### Step 3: Draft the High-Yield Follow-Up Plan

Goal: decide which follow-up idea families should be generated, deferred, or
quarantined.

Primary target:
`docs/rv64_gcc_torture_1000_pass_recovery/high_yield_followup_plan.md`

Actions:

- Evaluate the required follow-up families from the source idea.
- Rank candidate ideas by expected broad pass-count impact, ownership clarity,
  dependency order, and reuse of recent authority/freshness architecture.
- Explain why each high-yield candidate is not testcase-overfit.
- Mark unsettled architecture buckets as research or discussion candidates.
- Mark low-yield F128, string/library, builtin, or environment-dependent lanes
  as deferred or quarantined when appropriate.

Completion check:

- The plan names the follow-up ideas to generate or explicitly defer, gives
  expected breadth, and explains how the queue can plausibly move toward
  `1000+`.

### Step 4: Generate Ordered Follow-Up Ideas

Goal: create durable open ideas for the selected follow-up queue.

Primary target:
new files under `ideas/open/`

Actions:

- Create one idea per coherent owning layer and capability family.
- Include prerequisites, estimated evidence breadth, proof surface,
  acceptance criteria, and concrete reviewer reject signals in each idea.
- Use research/discussion idea shape for architecture weak points that should
  be settled before implementation.
- Avoid expanding the active umbrella source idea unless durable source intent
  genuinely changed.

Completion check:

- `ideas/open/` contains an ordered follow-up queue with no mixed-owner
  implementation routes and no testcase-shaped shortcuts.

### Step 5: Write the Dependency Order to 1000

Goal: make the recommended activation order explicit.

Primary target:
`docs/rv64_gcc_torture_1000_pass_recovery/dependency_order_to_1000.md`

Actions:

- Order follow-up ideas by producer-before-consumer dependencies and expected
  pass-count leverage.
- State which ideas must run before RV64 target consumers.
- State which deferred families are outside the first `1000+` route and why.
- Call out architecture weak points and their required discussion/research
  path.

Completion check:

- The dependency document gives a concrete next activation order and explains
  how it is intended to recover toward `1000+`.

### Step 6: Assemble the Handoff Index and Closure Notes

Goal: make the umbrella output navigable and ready for plan-owner closure
review.

Primary target:
`docs/rv64_gcc_torture_1000_pass_recovery/index.md`

Actions:

- Link all required handoff documents and generated follow-up ideas.
- Summarize the evidence used, bucket method, generated queue, high-yield
  route, architecture weak points, deferred families, stale-count guidance,
  and recommended next lifecycle activation.
- Verify the docs all agree on current `470/1467` evidence.

Completion check:

- The handoff directory has `index.md` plus all four required documents, and
  the index contains the closure-note information required by the source idea.

### Step 7: Final Lifecycle Readiness Check

Goal: decide whether the umbrella source idea is complete and ready to close.

Actions:

- Confirm every acceptance criterion in the source idea is satisfied.
- Confirm no implementation, expectation, unsupported-marker, allowlist,
  runtime, timeout, or harness changes were made.
- Confirm generated follow-up ideas carry reviewer reject signals.
- Ask the plan owner to close only after the source idea itself is satisfied;
  do not treat runbook exhaustion alone as closure.

Completion check:

- The supervisor can route closure review with clear evidence, generated
  follow-up ideas, and no implementation drift.
