Status: Active
Source Idea Path: ideas/open/295_backend_regex_failure_family_inventory.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Reconstruct the Post-301 Inventory

# Current Packet

## Just Finished

Lifecycle activation created an active runbook for idea 295 after idea 301
closed. No implementation or test files were touched.

## Suggested Next

Execute plan Step 1: reconstruct the post-301 backend-regex residual inventory
from `test_before.log`, confirming the accepted 352 selected / 300 passed / 52
failed state and recording the current failure list plus top-level
classification in this file.

## Watchouts

- This umbrella plan is classification-only; switch to a focused owner before
  implementation.
- Do not change expectations, allowlists, unsupported classifications, timeout
  policy, runner behavior, CTest registration, or test contracts.
- Do not reopen ideas 285 through 301 from counts alone; require
  generated-code, diagnostic, or proof evidence that contradicts closure
  boundaries.
- Keep timeout or output-storm cases separate unless evidence supports a
  hang-specific owner.

## Proof

Lifecycle-only activation. No build or runtime validation was run.
