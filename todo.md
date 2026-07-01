Status: Active
Source Idea Path: ideas/open/504_select_publication_move_bundle_evidence_authority.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect Select-Publication Evidence Surfaces

# Current Packet

## Just Finished

Idea 503 closed after Step 3 residual disposition. The single coherent
before-return `return_stack_to_register` row from idea 495 is now covered by a
focused RV64 object-emission path with fail-closed tests, including
value-identity keyed duplicate-load suppression.

Select-publication move-bundle evidence/authority remains outside idea 503 and
is now active under idea 504. Any downstream RV64 gcc-torture admission
reclassification or broader bucket refresh should be handled as a separate
lifecycle initiative if needed.

## Suggested Next

Execute Step 1 by inspecting the prepared/BIR select-publication record stream
and current move-bundle diagnostic publication for the three ambiguous rows.
Record the exact missing fields, producer surfaces, representative rows, and
whether idea 504 can publish evidence directly or must route a lower
prerequisite.

## Watchouts

- Do not re-open before-instruction, out-of-SSA, or before-return
  materialization under idea 504.
- Do not lower select-publication move bundles in RV64 before explicit
  authority is published.
- Do not infer authority from source names, absent case-log tokens, raw BIR
  shape, object output, final registers, or target behavior.
- Preserve existing untracked review artifacts and the rejected
  `test_baseline.new.log`.

## Proof

Lifecycle close/switch proof:

```sh
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_before.log --allow-non-decreasing-passed > test_after.log 2>&1 && git diff --check >> test_after.log 2>&1
```

Result: passed. Output preserved in `test_after.log`.
