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
  `/tmp/c4c-string-authority-isolate-51008`.
- Ran `python3 scripts/string_authority_guard.py` at every commit in the
  ordered range `e52189425..3486d702e`, plus the lower endpoint `e52189425`.
- The requested minimum commits were covered:
  `e52189425`, `3d2f4f903`, `61bbaf8eb`, `4976210ac`, `19b3b8765`,
  `6b5c0fa3f`, `eb4ebf5fc`, `c6fa8d8ee`, `f6bdf2641`, `e68d96749`, and
  `3486d702e`.
- Every tested commit passed with
  `string authority guard passed: 229 classified declaration-level hits`.
- No PASS-to-FAIL transition was found because the requested failing endpoint
  `3486d702e` passed when checked out in the historical worktree and run with
  the delegated guard command.
- Removed the temporary worktree after the sweep.

Preserved Step 2 classification:
- `c_testsuite_aarch64_backend_src_00040_c` did not reproduce as a timeout in
  the isolated Step 2 run and remains classified as timeout/noise unless a
  later broader run reproduces it.

## Suggested Next

Supervisor should reconcile the failing-endpoint assumption against the
historical script result: either rerun the guard at the originally failing
checkout/artifact that named `3486d702e`, or delegate a new isolation range
above `3486d702e` where the guard failure is known to reproduce.

## Watchouts

- Step 3 did not find a PASS-to-FAIL transition inside
  `e52189425..3486d702e`; all tested commits in that interval passed.
- This packet cannot isolate a first bad commit because the requested endpoint
  `3486d702e` did not fail under the delegated historical workflow.
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
git worktree add --detach /tmp/c4c-string-authority-isolate-51008 e52189425
python3 scripts/string_authority_guard.py
```

The script was run from the temporary worktree after checking out each tested
historical commit. `test_after.log` is the proof log path and records
`first_pass_to_fail_transition: not found in tested sequence` because all
tested commits passed.
