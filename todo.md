Status: Active
Source Idea Path: ideas/open/489_bir_dynamic_local_array_proof_population_from_lir_coordinates.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Residual Disposition For Idea 486

# Current Packet

## Just Finished

Completed Step 6 residual disposition after Step 5 populated
`local_array_proof_facts` from production range certificates. Idea 486 checker
input population can resume because proof facts now publish `Available` only
from matching `local_array_index_range_proofs` records and preserve
fail-closed status for missing/non-available certificates, selected-path-only
or interval-only inference, unsupported boundary, missing coordinate, clobber,
alias/phi ambiguity, unknown effect, non-covering path,
non-dominating/non-guarding proof, and coordinate confusion.

## Suggested Next

Ask the plan owner to close idea 489 and resume idea 486, which should consume
`local_array_proof_facts` as the proof-fact input surface for checker input
population.

## Watchouts

- Do not re-derive selected-path coverage, interval no-clobber, endpoint
  bridges, final homes, raw testcase shape, or synthetic effect inputs.
- Idea 486 should consume `local_array_proof_facts`, not lower
  `local_array_index_range_proofs` or raw selected-path/interval-effect
  records directly.
- Idea 484 packaging, scalar loads, and RV64/MIR lowering remain out of scope.

## Proof

Delegated Step 6 disposition proof:

```sh
git diff --check > test_after.log 2>&1 && python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_before.log --allow-non-decreasing-passed >> test_after.log 2>&1
```

Result: passed. Output preserved in `test_after.log`.
