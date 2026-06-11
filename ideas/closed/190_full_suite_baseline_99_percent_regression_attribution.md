# 190 Full-suite baseline 99 percent regression attribution

## Goal

Find the exact step where the full-suite baseline dropped from the accepted
100% result to the rejected 99% candidate, then decide whether the failures are
ambient noise, an implementation regression, or evidence that the current BIR
route-view design is exposing an unhandled semantic boundary.

## Why This Exists

The accepted `test_baseline.log` records a clean full-suite baseline:

- commit: `af8c2f6efac78a862e7f024cd579d1a2ec487be3`
- result: 3427/3427 passing

The newer `test_baseline.new.log` records a rejected full-suite candidate:

- commit: `9c1add9c93dcc9ddad8a3634cf725d14598aa48e`
- subject: `[backend] Migrate materialized condition branch to Route 7`
- result: 3424/3428 passing, 4 failing tests

Known failing tests in the rejected candidate:

- `c_testsuite_aarch64_backend_src_00040_c` timeout
- `c_testsuite_aarch64_backend_src_00119_c`
- `c_testsuite_aarch64_backend_src_00123_c`
- `c_testsuite_aarch64_backend_src_00195_c`

This should not be treated as a routine baseline refresh. The drop may be a
real regression introduced by a Phase E route-view migration, or it may expose
a design gap where route-first semantic views preserve narrow oracle tests but
fail broader AArch64 runtime behavior.

## In Scope

- Inspect baseline and proof logs in chronological order.
- Identify the last full-suite 100% baseline and the first full-suite 99%
  candidate.
- Correlate those logs with the ordered commit history and lifecycle closures
  around Phase E route-view migrations.
- Reproduce the four failing tests narrowly before making any repair.
- Determine whether the failure starts at Route 4, Route 5, Route 1, Route 2,
  Route 3, Route 6, Route 7, or a later cross-target reuse step.
- If the failure is real, isolate the semantic or target-policy boundary that
  the route-view migration mishandled.
- Produce a repair idea or implementation packet only after the first bad step
  and failure family are understood.

## Required Log Review

Review logs by filesystem timestamp and by commit order. Do not start from the
latest failure alone.

Minimum artifacts to inspect:

- `test_baseline.log`
- `test_baseline.new.log`
- `test_before.log`
- `test_after.log` if present
- close notes for `ideas/closed/182_phase_e_route4_publication_view_consumer_migration.md`
  through `ideas/closed/188_phase_e_route7_comparison_view_consumer_migration.md`
- active state for `ideas/open/189_phase_e_cross_target_route_view_interface_reuse.md`
- relevant `todo.md` baseline-review notes, especially commits that rejected a
  baseline candidate

Recommended investigation commands:

```sh
ls -lt test_baseline.log test_baseline.new.log test_before.log test_after.log 2>/dev/null || true
git log --oneline --date-order --decorate -30
git show --stat 9c1add9c93dcc9ddad8a3634cf725d14598aa48e
rg -n "test_baseline.new|baseline candidate|c_testsuite_aarch64_backend_src_00040_c|00119|00123|00195|99%|100%" ideas/closed ideas/open todo.md review docs -g '*.md'
```

If historical baseline logs exist under `log/`, sort them by timestamp and
compare the first appearance of the four-test failure family against the
current root logs.

## Out Of Scope

- Accepting `test_baseline.new.log` as the new baseline while it has fewer
  passing tests than the accepted baseline.
- Papering over the four failing tests with expectation downgrades,
  `unsupported` changes, timeout masking, or weaker contracts.
- Continuing broad Phase E retirement or aggregate contraction claims before
  the baseline drop is attributed.
- Treating this as only a flaky-test cleanup without proving whether the first
  bad step is repeatable.
- Repairing unrelated backend failures discovered during the audit unless they
  are part of the same first-bad failure family.

## Acceptance Criteria

- A chronological baseline timeline identifies:
  - the last known 100% full-suite baseline;
  - the first observed 99% full-suite candidate;
  - the commit or lifecycle slice where the drop first appears;
  - whether the four failing tests reproduce under a narrow command.
- The investigation distinguishes timeout/noise from deterministic backend
  behavior changes.
- If deterministic, the report explains which route-view migration or boundary
  exposed the failure and why the narrow proof did not catch it.
- Any repair proposal preserves the original c_testsuite contracts and does
  not weaken expectations.
- The final state either restores a 100% full-suite baseline candidate or
  opens a focused repair idea with the first-bad evidence attached.

## Proof Route

Start with no code changes.

1. Re-run only the four failing tests from `test_baseline.new.log` to classify
   deterministic failures versus timeout/noise.
2. If they reproduce, bisect manually across the Phase E route-view migration
   commits using the same narrow failing-test command.
3. After a candidate repair, run the four-test reproducer, the implicated
   route-view/oracle subset, and then a full-suite baseline candidate.
4. Accept a refreshed `test_baseline.log` only if the full-suite result is
   non-regressive relative to the accepted 100% baseline.

## Reviewer Reject Signals

- The route claims success by accepting or ignoring a 99% full-suite candidate.
- The investigation starts at the latest active idea and skips chronological
  log ordering.
- The first-bad commit is guessed from commit names instead of proven with logs
  or repro commands.
- The repair weakens c_testsuite expectations, marks supported behavior
  unsupported, or hides a timeout without explaining root cause.
- A narrow route-view proof is treated as enough even though the full-suite
  failure family still reproduces.
- The report dismisses the failures as ambient without repeat runs or
  historical evidence.

## Closure Note

Closed after completing the attribution and repair runbook.

The baseline timeline kept `af8c2f6efac78a862e7f024cd579d1a2ec487be3` as the
accepted 3427/3427 full-suite baseline and rejected the later 99% candidate
until its failures were attributed. The narrow reproducer separated the
`00040` timeout from the deterministic AArch64 failures in `00119`, `00123`,
and `00195`. Manual historical attribution proved
`c8346c7bb052af1ae81c7ca95bee1f71a899ea6d` as the first bad commit, implicating
Route 3 FP global-load identity at the boundary between semantic memory/source
identity and AArch64 prepared target-addressing policy.

The focused repair restored the FP same-block global-load prepared fallback
without weakening c_testsuite contracts or accepting the rejected 99% baseline.
The final full-suite baseline candidate at
`e5218942517a2bb9c3bd0167d810e27bed8273c8` passed 3428/3428 and was accepted
as the repaired baseline. Close-gate backend regression logs used matching
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`
commands; the guard passed with 180/180 before and 180/180 after, with no new
failures.
