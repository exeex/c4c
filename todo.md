Status: Active
Source Idea Path: ideas/open/199_full_suite_baseline_string_authority_timeout_attribution.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Isolate the First Bad String-Authority Step

# Current Packet

## Just Finished

Completed Step 3: Isolate the First Bad String-Authority Step.

Historical isolation results:
- Used a temporary worktree outside the active checkout:
  `/tmp/c4c-string-authority-isolate-48163`.
- Ran `python3 scripts/string_authority_guard.py` at every commit in the
  ordered range `3486d702e..521cf148f`, plus the lower endpoint `3486d702e`.
- The requested minimum commits were covered:
  `3486d702e`, `6f5b01955`, `23821ceb8`, `55125bd83`, `e58237c1f`,
  `9732ff7c8`, `f281426eb`, and `521cf148f`.
- Every tested commit failed with `string authority guard failed`.
- The first observed bad commit in this bounded check is `3486d702e`, but this
  packet cannot identify a first bad commit between `3486d702e` and
  `521cf148f` because the lower endpoint is already failing.
- Common diagnostic:
  `src/backend/mir/aarch64/codegen/calls.cpp:8062:
  find_prepared_indirect_callee_stored_value_source_fallback:
  pattern=lookup-helper-rendered-compatibility: classify this exact
  path+symbol in scripts/string_authority_classifications.json or replace it
  with structured authority`.
- Removed the temporary worktree after the sweep.

Preserved Step 2 classification:
- `c_testsuite_aarch64_backend_src_00040_c` did not reproduce as a timeout in
  the isolated Step 2 run and remains classified as timeout/noise unless a
  later broader run reproduces it.

## Suggested Next

Supervisor should decide whether to extend historical isolation before
`3486d702e`, treat `3486d702e` as the attribution boundary for this plan, or
route to a repair packet for the reported string-authority diagnostic.

## Watchouts

- Step 3 did not find a PASS-to-FAIL transition inside
  `3486d702e..521cf148f`; all tested commits in that interval failed.
- Do not infer first-bad from `1d1c506f0`; Step 1 found
  `string_authority_guard` already failing in
  `log/baseline_521cf148fa7660766f5bfa8c0932833bfa987311.log`.
- The Step 2 isolated proof reproduced only `string_authority_guard`; `00040`
  passed and should not drive the next route unless it reappears in broader
  validation.
- Do not weaken, disable, or bypass `string_authority_guard`.
- Do not delete, rename, or broadly rewrite Phase D or Phase E docs just to
  satisfy the guard.
- Do not open draft 155 or proceed with true Phase E retirement under this
  attribution plan.
- Missing artifact for exact first-bad: no full-suite archived baseline exists
  between `3486d702e` at 12:34 and `521cf148f` at 13:39.

## Proof

Ran the supervisor-delegated historical proof workflow and captured the commit
table in `test_after.log`:

```sh
git worktree add --detach /tmp/c4c-string-authority-isolate-48163 3486d702e
python3 scripts/string_authority_guard.py
```

The script was run from the temporary worktree after checking out each tested
historical commit. `test_after.log` is the proof log path and records
`first_observed_bad: 3486d702e` plus the explicit lower-endpoint blocker to
identifying a first bad commit inside the requested interval.
