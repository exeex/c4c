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
  `/tmp/c4c-string-authority-above-xA6sxb/wt`.
- Ran `(cd temporary worktree && python3 scripts/string_authority_guard.py)` at
  `3486d702e`, every commit in
  `git log --oneline --reverse 3486d702e..521cf148f`, and `521cf148f`.
- Found the first real PASS-to-FAIL transition above `3486d702e`:
  - preceding passing commit:
    `1496a715d` (`[todo_only] Record Route 3 consumer inventory`)
  - first bad commit:
    `9e4892bcd` (`Clarify Route 3 indirect callee fallback boundary`)
- Passing commits through `1496a715d` reported
  `string authority guard passed: 229 classified declaration-level hits`.
- `9e4892bcd` and every later tested commit through `521cf148f` reported
  `string authority guard failed`.
- Removed the temporary worktree after the sweep.

Preserved Step 2 classification:
- `c_testsuite_aarch64_backend_src_00040_c` did not reproduce as a timeout in
  the isolated Step 2 run and remains classified as timeout/noise unless a
  later broader run reproduces it.

## Suggested Next

Supervisor should route the next packet to inspect the first-bad diff at
`9e4892bcd` against preceding pass `1496a715d`, identify the exact guarded
string-authority violation introduced by
`Clarify Route 3 indirect callee fallback boundary`, and decide whether repair
belongs in this plan or a focused follow-up idea.

## Watchouts

- The lower endpoint `3486d702e` still passes under the corrected fresh
  worktree method, matching the previous lower-range sweep.
- The first bad commit is `9e4892bcd`; do not infer first-bad from later
  failing commits such as `521cf148f` or the rejected candidate `1d1c506f0`.
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

Ran the supervisor-delegated historical proof workflow from inside a fresh
temporary worktree and captured the commit table in `test_after.log`:

```sh
git worktree add --detach /tmp/c4c-string-authority-above-xA6sxb/wt 3486d702e
(cd /tmp/c4c-string-authority-above-xA6sxb/wt && python3 scripts/string_authority_guard.py)
```

The script was run from the temporary worktree after checking out each
historical commit in the delegated range. `test_after.log` is the proof log
path and records
`first_pass_to_fail_transition: 1496a715d -> 9e4892bcd`.
