Status: Active
Source Idea Path: ideas/open/492_dynamic_local_array_selected_proof_edge_path_record_status_api.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Selected Proof-Edge Record Homes

# Current Packet

## Just Finished

Lifecycle split closed idea 491 as a routed blocker investigation and
activated idea 492 for the first missing lower owner: a durable selected
proof-edge path record/status API keyed to `lir_producer_lookup_key`.

## Suggested Next

Execute Step 1: audit prepared record/status surfaces, selected local-array
path records, prepared branch/compare facts, and helper inputs for the narrow
record/status API. Record the exact candidate home or next lower blocker.

## Watchouts

- Keep dynamic-index interval effect/no-clobber classification out of this
  runbook; it remains a later owner.
- Do not populate idea 489 proof facts or idea 486 checker inputs directly.
- Do not treat helper-local reachability/dominance queries as durable proof
  facts unless this runbook publishes explicit records/statuses.
- Keep `lir_producer_instruction_index` as a LIR producer-site coordinate.
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
reports Step 1, `Audit Selected Proof-Edge Record Homes`.
