Status: Active
Source Idea Path: ideas/open/491_dynamic_local_array_selected_proof_edge_path_certificate.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Selected Proof-Edge Path Certificate Inputs

# Current Packet

## Just Finished

Lifecycle split closed idea 490 as a routed certificate investigation and
activated idea 491 for the first missing lower owner: selected proof-edge path
certification keyed to `lir_producer_*`.

## Suggested Next

Execute Step 1: audit prepared branch/compare facts, selected edge/outcome
surfaces, dominance/reachability helpers, `lir_producer_*` keys, and same-block
ordering boundaries. Record whether a bounded selected proof-edge path
certificate producer exists or name the exact lower blocker.

## Watchouts

- Keep dynamic-index interval effect/no-clobber classification out of this
  runbook; it is a later owner after selected path certification.
- Do not populate idea 489 proof facts or idea 486 checker inputs directly.
- Keep `lir_producer_instruction_index` as a LIR producer-site coordinate.
- Do not infer proof edges or path coverage from branch proximity, loop shape,
  value names, testcase names, dump order, final homes, or target behavior.
- Existing untracked `review/*.md` files are transient and must remain
  untouched.

## Proof

Lifecycle activation validation:

```sh
git diff --check
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_before.log --allow-non-decreasing-passed
python3 scripts/plan_review_state.py show
```

Result: passed. Regression sanity used the unchanged canonical log as a
lifecycle-only close gate (`328/328` before and after). Hook-backed state now
reports Step 1, `Audit Selected Proof-Edge Path Certificate Inputs`.
