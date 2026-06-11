Status: Active
Source Idea Path: ideas/open/199_full_suite_baseline_string_authority_timeout_attribution.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Build the Baseline and Commit Timeline

# Current Packet

## Just Finished

Completed Step 1: Build the Baseline and Commit Timeline.

Accepted baseline authority:
- `test_baseline.log`, mtime 2026-06-11 11:11, records `100% tests passed,
  0 tests failed out of 3428`.
- In that accepted run, `string_authority_guard` passed in 3.02 sec and
  `c_testsuite_aarch64_backend_src_00040_c` passed in 4.88 sec.

Rejected current candidate:
- `test_baseline.new.log`, mtime 2026-06-11 14:49, matches
  `log/baseline_1d1c506f05dbaf783f6293d61e4b05d339395c4e.log`.
- Commit `1d1c506f0` is `Write lifecycle naming cleanup note`; `git show
  --stat` shows it added
  `docs/bir_prealloc_fusion/phase_d_phase_e_lifecycle_naming_cleanup.md` and
  rewrote `todo.md` only.
- The candidate records `99% tests passed, 2 tests failed out of 3428`:
  `string_authority_guard` failed and
  `c_testsuite_aarch64_backend_src_00040_c` timed out.

Nearby root proof logs:
- `test_before.log` and `test_after.log` both predate the rejected candidate
  and both record `100% tests passed, 0 tests failed out of 180`; they are not
  full-suite baseline evidence for idea 199.

Recent archived baseline timeline:
- 2026-06-11 00:17 `9dc99117c`:
  `99% tests passed, 2 tests failed out of 3428`; failures were `00119` and
  `00195`, not `string_authority_guard` or `00040`.
- 2026-06-11 01:22 through 08:15 (`5701c3f3d`, `496861106`,
  `ff5ef57eb`, `688711ba8`, `221ef0dd2`, `d69db3e73`):
  `99% tests passed, 3 tests failed out of 3428`; failures were `00119`,
  `00123`, and `00195`, not `string_authority_guard` or `00040`.
- 2026-06-11 10:06 `9c1add9c9`:
  `99% tests passed, 4 tests failed out of 3428`; first recent observed
  `00040` timeout, plus `00119`, `00123`, and `00195`.
- 2026-06-11 12:34 `3486d702e`:
  `99% tests passed, 1 tests failed out of 3428`; only `00040` timed out.
- 2026-06-11 13:39 `521cf148f`:
  `99% tests passed, 2 tests failed out of 3428`; first recent observed
  `string_authority_guard` failure, with `00040` timeout.
- 2026-06-11 14:49 `1d1c506f0`:
  same two-failure shape as `521cf148f`.

Candidate ranges:
- `00040` first recent observed timeout is between `d69db3e73` and
  `9c1add9c9`; this remains separate from the later string-authority route.
- `string_authority_guard` first recent observed failure is between
  `3486d702e` and `521cf148f`.
- There are no archived `log/baseline_*.log` files between the 12:34
  `3486d702e` baseline and the 13:39 `521cf148f` baseline, so Step 1 cannot
  identify the exact first bad commit without Step 3 narrow/manual isolation.

Plausible first-bad commits or artifacts for the string-authority range:
- `6f5b01955` through `23821ceb8`: residual route-view lifecycle, review, and
  `docs/bir_prealloc_fusion/residual_route_view_consumer_migration_map.md`.
- `e58237c1f` through `9732ff7c8`: Route 3 boundary plan, including
  implementation/test commits `9e4892bcd` and `74c8a6335`; these are less
  obviously string-authority-shaped but fall inside the unproven window.
- `f281426eb` through `521cf148f`: prepared diagnostics oracle planning and
  `docs/bir_prealloc_fusion/prepared_diagnostics_oracle_replacement_plan.md`;
  the first observed guard failure is at the close commit `521cf148f`.
- `1d1c506f0` and
  `docs/bir_prealloc_fusion/phase_d_phase_e_lifecycle_naming_cleanup.md` are
  latest rejected-candidate artifacts, but not proven first-bad because the
  same guard failure is already present at `521cf148f`.

Search evidence:
- The idea-199 search string hits `string_authority_guard`,
  `test_baseline.new`, `3426/3428`, `00040`, `99%`, `baseline candidate`, and
  `string authority` primarily in the active source idea and current
  `todo.md`.
- Closed ideas 191-198 and the durable Phase D/Phase E docs contain extensive
  Phase D versus true Phase E lifecycle language, but targeted searches did
  not find `string_authority_guard`, `test_baseline.new`, `3426/3428`, `99%`,
  or `baseline candidate` in those artifacts.
- Historical `log/baseline_*.log` contains older `string_authority_guard`
  failures outside the current Phase D follow-up sequence, so older archive
  hits are not enough to identify this sequence's first bad step.

## Suggested Next

Execute Step 2: reproduce and classify `string_authority_guard` and
`c_testsuite_aarch64_backend_src_00040_c` separately, using supervisor-delegated
narrow commands and preserving canonical proof output.

## Watchouts

- Do not accept `test_baseline.new.log` while it is regressive against the
  accepted 3428/3428 baseline.
- Do not infer first-bad from `1d1c506f0`; `string_authority_guard` is already
  failing in `log/baseline_521cf148fa7660766f5bfa8c0932833bfa987311.log`.
- Keep `string_authority_guard` and `00040` classification separate:
  `00040` appears earlier in the recent timeline than the guard failure.
- Do not weaken, disable, or bypass `string_authority_guard`.
- Do not delete, rename, or broadly rewrite Phase D or Phase E docs just to
  satisfy the guard.
- Do not open draft 155 or proceed with true Phase E retirement under this
  attribution plan.
- Missing artifact for exact first-bad: no full-suite archived baseline exists
  between `3486d702e` at 12:34 and `521cf148f` at 13:39.

## Proof

Analysis-only packet. Per delegation, no build or tests were run and no
baseline/proof log was modified. Read-only evidence came from `ls -lt
test_baseline.log test_baseline.new.log test_before.log test_after.log`,
`git log --oneline --date-order --decorate -40`, `git show --stat
1d1c506f05dbaf783f6293d61e4b05d339395c4e`, targeted `rg` searches, archived
`log/baseline_*.log` inspection, and commit-range history queries.
