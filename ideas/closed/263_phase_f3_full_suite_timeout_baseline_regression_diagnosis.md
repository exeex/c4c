# 263 Phase F3 full-suite timeout baseline regression diagnosis

## Goal

Diagnose and repair the full-suite baseline regression where
`test_baseline.new.log` dropped from 3428/3428 to 3427/3428 because
`c_testsuite_aarch64_backend_src_00040_c` timed out.

This is a repair and diagnosis idea. It must determine whether the timeout is
environmental/flaky or exposes a real backend/runtime design gap introduced
during the recent Phase F3 convergence work.

## Why This Exists

The accepted baseline in `test_baseline.log` reports:

- 100% tests passed;
- 3428/3428 passing.

The hook-produced candidate baseline in `test_baseline.new.log` reports:

- 99% tests passed;
- 3427/3428 passing;
- failing test:
  `1560 - c_testsuite_aarch64_backend_src_00040_c (Timeout)`.

The candidate baseline was rejected and retained for diagnosis. Do not accept
or delete it until the root cause is understood and a fresh matching proof is
available.

## Required First Step: Timeline Log Audit

Before changing code, inspect logs and lifecycle history in chronological
order. The diagnosis must identify the last step that still had a 100%
baseline and the first later step where the full-suite candidate dropped to
99%.

At minimum, inspect:

- `test_baseline.log`;
- `test_baseline.new.log`;
- recent `git log --oneline --decorate` entries around ideas 248 through 262;
- closure notes for ideas 248 through 262;
- any canonical `test_before.log` / `test_after.log` evidence referenced by
  the closure notes;
- `log/` baseline artifacts, if present, sorted by commit/time.

The closure note must record the time-ordered sequence and the exact point
where the full-suite baseline first changed from 3428/3428 to 3427/3428, or
state why the available logs cannot identify that point.

## In Scope

- Reproducing the failing test narrowly:
  `c_testsuite_aarch64_backend_src_00040_c`.
- Comparing behavior against the accepted full-suite baseline.
- Checking whether the timeout is deterministic, data-dependent, or a stale
  runtime/process artifact.
- Inspecting recent Phase F3 changes that could affect AArch64 runtime,
  c-testsuite execution, selected `LoadLocal`, compare-join, store-source,
  publication, module structural helpers, or target output.
- Fixing the underlying issue if a real regression is found.
- Producing a fresh matching proof log after the fix.

## Out Of Scope

- Accepting `test_baseline.new.log` while it reports 3427/3428.
- Hiding the timeout by weakening tests, increasing timeout blindly, changing
  supported status, or excluding the test.
- Broad prepared/BIR retirement or unrelated portability cleanup.
- Treating focused Phase F3 proof logs as sufficient to accept the full-suite
  baseline regression.

## Acceptance Criteria

- The last known 3428/3428 point and first known 3427/3428 point are recorded
  from logs or commit history.
- The failing timeout is reproduced or convincingly classified as
  non-reproducible environmental flake with supporting reruns.
- If a real regression exists, the patch fixes the root cause rather than
  masking the timeout.
- `test_baseline.new.log` is either replaced by a fresh 3428/3428 candidate
  and accepted through the supervisor baseline flow, or remains rejected with a
  documented blocker.
- Any fix preserves the Phase F3 ownership boundaries: no expectation
  weakening, no prepared compatibility hiding, no target-policy migration into
  BIR, and no broad retirement claims.

## Suggested Proof

Start narrow, then broaden only after diagnosis:

```bash
ctest --test-dir build -R '^c_testsuite_aarch64_backend_src_00040_c$' --output-on-failure
```

If a fix lands, run a matching before/after proof for the narrow failing test
and then a supervisor-approved broader baseline or full-suite proof sufficient
to restore 3428/3428 confidence.

## Reviewer Reject Signals

- The investigation skips the time-ordered log audit.
- The fix only changes timeout, labels, expected output, supported status, or
  baseline acceptance without explaining the underlying cause.
- The closure claims full-suite health from focused 2/2 or 3/3 Phase F3 proof
  logs while `test_baseline.new.log` still reports 3427/3428.
- The patch expands prepared/BIR portability work instead of repairing the
  baseline regression.
- The diagnosis deletes the failing candidate log before extracting the
  failing test and timeline evidence.

## Close Note

Closed on 2026-06-14. The diagnosis runbook completed and found no concrete
semantic/backend regression to repair.

Timeline audit:

- Last known 3428/3428 baseline: `ab6efaf8a`, recorded in
  `test_baseline.log` and
  `log/baseline_ab6efaf8a3af38595cd8a4bdb64932b49fcea680.log`.
- First later known 3427/3428 candidate: `b9d71a7a`, recorded in the stale
  `test_baseline.new.log` and
  `log/baseline_b9d71a7a9e10554c0496c50e82209ac6a3db9d06.log`.
- Transition classification: bounded, not exact. No full-suite baseline
  artifacts were available for intervening commits `728856001` through
  `b6353fe6d`.

Narrow reproduction result:

- `ctest --test-dir build -R '^c_testsuite_aarch64_backend_src_00040_c$' --output-on-failure`
  did not reproduce the timeout in narrow reruns.
- The narrow rerun passed in approximately 2.85 seconds.
- This evidence classified the timeout as not reproduced in isolation, not as
  fixed by an implementation change.

Root-cause classification:

- No concrete semantic/backend root cause was identified.
- The best-supported classification is a load-sensitive or environmental
  full-suite timeout margin.
- The bounded implementation diffs touched prepared prealloc lookup paths, but
  no c-testsuite registration, timeout policy, AArch64 emitter, runner, or
  `00040.c` source changes explained the stale 3427/3428 candidate.
- Step 4 repair was intentionally skipped because there was no diagnosed real
  regression to patch.

Full-suite proof and baseline acceptance:

- Supervisor full-suite proof command:
  `ctest --test-dir build -j --output-on-failure 2>&1 | tee test_after.log`.
- `test_after.log` records `100% tests passed, 0 tests failed out of 3428`.
- `c_testsuite_aarch64_backend_src_00040_c` passed in 3.26 seconds in that
  full-suite run.
- The stale 3427/3428 candidate was replaced by the fresh 3428/3428 candidate
  and accepted through `scripts/plan_review_state.py accept-baseline`.
- `test_baseline.log` now records the accepted 3428/3428 baseline, and
  `test_baseline.new.log` is absent after acceptance.

Close-time regression guard:

- Ran
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_baseline.log --after test_after.log --allow-non-decreasing-passed`.
- Result: PASS, with 3428/3428 before and 3428/3428 after, no new failures,
  and no new tests over the 30 second reporting threshold.

No implementation files changed for this idea, no test contract was weakened,
and no timeout, label, supported-status, or expected-output change was used to
claim progress.
