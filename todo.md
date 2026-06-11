Status: Active
Source Idea Path: ideas/open/190_full_suite_baseline_99_percent_regression_attribution.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Build The Baseline Timeline

# Current Packet

## Just Finished

Completed `plan.md` Step 1, "Build The Baseline Timeline", as a
read-only investigation packet.

Timestamp-aware baseline timeline:

- 2026-06-10 22:04:42 UTC: `test_baseline.log` and
  `log/baseline_af8c2f6efac78a862e7f024cd579d1a2ec487be3.log` record the
  accepted full-suite baseline at `af8c2f6ef Add Route 2 scalar-ineligible
  oracle`: `100% tests passed, 0 tests failed out of 3427`.
- 2026-06-10 23:01:37 UTC:
  `log/baseline_76bfe950d86dc35d9fdd848c8322023a6105ef26.log`, mapped to
  `76bfe950d Close Route 4 block-entry publication migration`, is the first
  historical post-`af8c2f6` baseline candidate found under `log/`. It is
  regressive: `99% tests passed, 4 tests failed out of 3427`. Its failure set
  is `backend_aarch64_instruction_dispatch`,
  `c_testsuite_aarch64_backend_src_00119_c`,
  `c_testsuite_aarch64_backend_src_00123_c`, and
  `c_testsuite_aarch64_backend_src_00195_c`.
- 2026-06-11 00:17:49 UTC:
  `log/baseline_9dc99117c6604ffad94c64d61cf2b378be0175b4.log`, mapped to
  `9dc99117c Project value-home lookups separately`, records `99%`, 2/3428
  failing: `00119` and `00195`.
- 2026-06-11 01:22:40 UTC through 2026-06-11 08:15:00 UTC: historical
  candidates for `5701c3f3d`, `496861106`, `ff5ef57eb`, `688711ba8`,
  `221ef0dd2`, and `d69db3e73` all record `99%`, 3/3428 failing:
  `00119`, `00123`, and `00195`.
- 2026-06-11 10:06:09 UTC: `test_baseline.new.log` and
  `log/baseline_9c1add9c93dcc9ddad8a3634cf725d14598aa48e.log`, mapped to
  `9c1add9c9 [backend] Migrate materialized condition branch to Route 7`,
  record the rejected candidate: `99% tests passed, 4 tests failed out of
  3428`. Its failure set is `c_testsuite_aarch64_backend_src_00040_c`
  timeout plus `00119`, `00123`, and `00195` failures.
- 2026-06-11 10:41:25 UTC and 2026-06-11 10:41:51 UTC: current
  `test_before.log` and `test_after.log` are newer backend regression-guard
  artifacts, both `180/180` passing. They are not full-suite baseline
  candidates and do not replace the baseline bracket.

Lifecycle/history evidence:

- `ideas/closed/182_phase_e_route4_publication_view_consumer_migration.md`
  explicitly rejects a hook-produced `test_baseline.new.log` because it
  regressed `00119`, `00123`, and `00195`.
- `5806b57a3 [todo_only] [todo] Reject regressive baseline candidate` records
  the rejection during the Route 7 lifecycle state.
- Recent ancestry from the accepted baseline to the rejected candidate includes
  Phase E Route 4, Route 5, Route 1, Route 2, Route 3, Route 6, and Route 7
  lifecycle and implementation slices. The current evidence bracket is
  `af8c2f6efac78a862e7f024cd579d1a2ec487be3` good to the first observed
  post-baseline candidate `76bfe950d86dc35d9fdd848c8322023a6105ef26`
  regressive, with later candidates preserving a recurring `00119`/`00195`
  core and often `00123`. Do not infer the first-bad commit from timestamps
  alone; Step 2/Step 3 still need repro and consistent cross-commit proof.

## Suggested Next

Execute `plan.md` Step 2: run the fixed four-test reproducer for
`c_testsuite_aarch64_backend_src_00040_c`, `00119`, `00123`, and `00195` to
classify timeout/noise versus deterministic failures before any repair or
baseline refresh.

## Watchouts

- The earliest historical regressive candidate found after the accepted
  baseline is `76bfe950d`, not `9c1add9c`, but it includes an internal
  `backend_aarch64_instruction_dispatch` failure instead of the later `00040`
  timeout. Treat this as an evidence gap to classify, not a first-bad answer.
- `00119` and `00195` appear in every post-`af8c2f6` regressive candidate
  inspected here; `00123` appears in most; `00040` appears only in the rejected
  `9c1add9c` candidate among the inspected full-suite logs.
- Keep `test_baseline.new.log` rejected until a non-regressive full-suite
  candidate exists. Do not weaken c_testsuite expectations.

## Proof

Investigation-only packet; no build/tests run and no logs modified. Read-only
evidence commands used: `ls -lt --full-time` for root logs, `find log` with
mtime sorting, `git log`, `git show`, and `rg` over baseline logs, lifecycle
notes, review notes, and docs. No `test_after.log` was written for this
mapping-only packet.
