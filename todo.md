Status: Active
Source Idea Path: ideas/open/486_bir_index_range_proof_path_dominance_carrier.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Populate Checker Inputs From Proof Facts

# Current Packet

## Just Finished

Lifecycle switched from completed idea 489 back to reopened idea 486. Idea 489
closed after publishing `local_array_proof_facts`; idea 486 can now resume
checker input population from that proof-fact surface.

## Suggested Next

Implement Step 5 by populating production checker inputs from matching
`local_array_proof_facts`, then prove available and fail-closed checker records
through focused backend coverage.

## Watchouts

- Consume `local_array_proof_facts`; do not re-derive from
  `local_array_index_range_proofs`, selected paths, interval effects, endpoint
  bridges, final homes, raw testcase shape, or synthetic effect inputs.
- Preserve fail-closed status distinctions for missing/non-available facts,
  selected-path-only or interval-only inference, unsupported boundary, missing
  coordinate, clobber, alias/phi, unknown effect, non-covering path,
  non-dominating/non-guarding proof, and coordinate confusion.
- Idea 484 packaging, scalar loads, RV64/MIR lowering, and broad generic range
  analysis remain out of scope.

## Proof

Lifecycle switch proof:

```sh
git diff --check > test_after.log 2>&1 && python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_before.log --allow-non-decreasing-passed >> test_after.log 2>&1
```

Result: passed. Output preserved in `test_after.log`.
