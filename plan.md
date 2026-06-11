# Full-suite Baseline Regression Attribution Runbook

Status: Active
Source Idea: ideas/open/190_full_suite_baseline_99_percent_regression_attribution.md

## Purpose

Attribute the drop from the accepted 3427/3427 full-suite baseline to the
rejected 3424/3428 candidate before any repair, baseline refresh, or Phase E
retirement work continues.

## Goal

Identify the first step where the four-test AArch64 c_testsuite failure family
appears, classify the failures as noise or deterministic regression, and route
any repair from proven evidence.

## Core Rule

Start with investigation only. Do not accept a 99% baseline, weaken c_testsuite
contracts, mark tests unsupported, hide timeouts, or make code repairs before
the first-bad step and failure family are understood.

## Read First

- `ideas/open/190_full_suite_baseline_99_percent_regression_attribution.md`
- `test_baseline.log`
- `test_baseline.new.log`
- `test_before.log`
- `test_after.log` if present
- close notes for `ideas/closed/182_phase_e_route4_publication_view_consumer_migration.md`
  through `ideas/closed/188_phase_e_route7_comparison_view_consumer_migration.md`
- close notes for `ideas/closed/189_phase_e_cross_target_route_view_interface_reuse.md`
- relevant `review/*.md`, `docs/*.md`, and lifecycle notes only when they mention
  baseline candidates or the four failing tests

## Current Targets

- Last known full-suite 100% baseline:
  `af8c2f6efac78a862e7f024cd579d1a2ec487be3`, 3427/3427 passing.
- Rejected full-suite candidate:
  `9c1add9c93dcc9ddad8a3634cf725d14598aa48e`, 3424/3428 passing.
- Known rejected-candidate failures:
  - `c_testsuite_aarch64_backend_src_00040_c` timeout
  - `c_testsuite_aarch64_backend_src_00119_c`
  - `c_testsuite_aarch64_backend_src_00123_c`
  - `c_testsuite_aarch64_backend_src_00195_c`

## Non-Goals

- Do not refresh `test_baseline.log` to a regressive result.
- Do not downgrade expectations, add unsupported markers, or narrow test
  contracts to hide the four-test family.
- Do not continue broad Phase E retirement or aggregate contraction claims until
  the baseline drop is attributed.
- Do not repair unrelated backend failures unless they are proven to share the
  same first-bad failure family.
- Do not treat narrow route-view oracle coverage as acceptance proof while the
  full-suite failure family still reproduces.

## Working Model

The accepted and rejected full-suite logs bracket a possible Phase E regression.
The investigation must order logs by filesystem time and by commit history,
then use a stable narrow reproducer for the four failing tests to determine
whether the failure is repeatable. Only after that should execution bisect
across Route 4, Route 5, Route 1, Route 2, Route 3, Route 6, Route 7, and later
cross-target reuse slices.

## Execution Rules

- Keep routine investigation results in `todo.md`.
- Preserve root canonical log names: `test_before.log` and `test_after.log`.
- If extra scratch logs are unavoidable, keep them out of the repo root unless
  the supervisor explicitly chooses them as canonical artifacts.
- Use the same narrow failing-test command when comparing commits.
- Treat test timeouts as unclassified until repeat runs or historical evidence
  distinguish noise from deterministic behavior.
- If deterministic regression is proven, open or hand off a focused repair
  packet only after recording first-bad evidence.

## Step 1: Build The Baseline Timeline

Goal: Establish the chronological evidence before reproducing or repairing.

Primary targets:
- root baseline logs
- historical logs under `log/` if present
- Phase E closed-idea notes and recent commit history

Actions:
- Inspect timestamps for `test_baseline.log`, `test_baseline.new.log`,
  `test_before.log`, and `test_after.log` when present.
- Sort any historical baseline logs under `log/` by timestamp.
- Read commit history around the accepted baseline commit and rejected candidate
  commit.
- Search lifecycle, review, and docs notes for the rejected tests and baseline
  candidate language.
- Record the last known 100% full-suite baseline, first observed 99% candidate,
  and any intermediate candidate evidence in `todo.md`.

Suggested commands:

```sh
ls -lt test_baseline.log test_baseline.new.log test_before.log test_after.log 2>/dev/null || true
find log -type f 2>/dev/null | sort
git log --oneline --date-order --decorate -40
git show --stat 9c1add9c93dcc9ddad8a3634cf725d14598aa48e
rg -n "test_baseline.new|baseline candidate|c_testsuite_aarch64_backend_src_00040_c|00119|00123|00195|99%|100%" ideas/closed ideas/open todo.md review docs -g '*.md'
```

Completion check:
- `todo.md` contains a timestamp-aware baseline timeline and identifies the
  current evidence bracket without guessing the first-bad commit.

## Step 2: Classify The Four-test Reproducer

Goal: Determine whether the rejected-candidate failures reproduce under a
narrow, repeatable command before any repair work.

Primary targets:
- `c_testsuite_aarch64_backend_src_00040_c`
- `c_testsuite_aarch64_backend_src_00119_c`
- `c_testsuite_aarch64_backend_src_00123_c`
- `c_testsuite_aarch64_backend_src_00195_c`

Actions:
- Build once before running the reproducer.
- Run only the four known failing tests with a stable CTest regex.
- Repeat failures enough to classify timeout/noise versus deterministic backend
  behavior.
- Preserve the exact command and outcome in `todo.md`.

Suggested command:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(c_testsuite_aarch64_backend_src_00040_c|c_testsuite_aarch64_backend_src_00119_c|c_testsuite_aarch64_backend_src_00123_c|c_testsuite_aarch64_backend_src_00195_c)$'
```

Completion check:
- `todo.md` records whether each failing test reproduces, flakes, times out, or
  passes under the narrow command.

## Step 3: Attribute The First Bad Step

Goal: Identify the commit or lifecycle slice where the failure family first
appears.

Primary targets:
- Phase E Route 4 through Route 7 migration commits
- later cross-target route-view reuse commits
- lifecycle closures for ideas 182 through 189

Actions:
- Use the Step 2 narrow reproducer consistently across candidate commits.
- Manually bisect across the Phase E route-view migration order.
- Correlate repro results with lifecycle closure notes and proof logs.
- Decide whether the first bad point is Route 4, Route 5, Route 1, Route 2,
  Route 3, Route 6, Route 7, or a later reuse step.
- Explain why previous narrow proofs missed the full-suite failure family.

Completion check:
- `todo.md` names the first bad commit or slice, the exact evidence command,
  and whether the failure is deterministic or noise.

## Step 4: Route The Repair Or Baseline Decision

Goal: Convert attribution into the smallest correct next action.

Actions:
- If the failure is ambient noise, document repeat-run and historical evidence
  strong enough to justify that classification.
- If deterministic, identify the mishandled semantic or target-policy boundary
  exposed by the migration.
- Either prepare a focused implementation packet for the implicated boundary or
  create a new repair idea under `ideas/open/` if the repair is a separate
  initiative.
- Keep c_testsuite contracts unchanged.

Completion check:
- The next action is evidence-backed and does not rely on accepting the 99%
  baseline candidate.

## Step 5: Prove The Final State

Goal: Accept only a non-regressive full-suite outcome or leave durable first-bad
evidence for a focused repair initiative.

Actions:
- After any candidate repair, rerun the four-test reproducer.
- Run the implicated route-view or backend oracle subset.
- Run a full-suite baseline candidate before refreshing `test_baseline.log`.
- Accept a refreshed baseline only if it is non-regressive relative to the
  accepted 3427/3427 result.

Completion check:
- The final state either restores a 100% full-suite baseline candidate or opens
  a focused repair idea with first-bad evidence attached.
