# Backend Regex Failure Family Inventory Todo

Status: Active
Source Idea Path: ideas/open/295_backend_regex_failure_family_inventory.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh The Remaining Backend Surface

# Current Packet

## Just Finished

Lifecycle closed focused idea 296 as complete for the fused compare-branch
operand-form owner and switched active state back to umbrella inventory idea
295. The focused close proof is the accepted `test_before.log` result for the
27-test scope: 23 passed / 4 failed.

## Suggested Next

Refresh or review the remaining backend-regex evidence from the main build
tree, then classify the residual buckets before splitting any new focused
owner.

## Watchouts

- `00200` is currently classified as a runtime mismatch.
- `00207`, `00214`, and `00215` are currently classified as scalar add/xor
  immediate printer limits.
- Do not reopen closed owners 285 through 296 from counts alone.
- Do not implement backend fixes while this umbrella inventory is active.
- Do not change expectations, allowlists, unsupported classifications, CTest
  registration, timeout policy, or runner behavior.

## Proof

Close-time lifecycle guard used the existing accepted focused proof in
`test_before.log` and ran:

`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_before.log --allow-non-decreasing-passed`

Result: PASS. The log reports 23 passed / 4 failed out of 27, with no new
failures because this was a lifecycle-only close decision over already
committed implementation slices.
