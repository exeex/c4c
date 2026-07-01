Status: Active
Source Idea Path: ideas/open/490_dynamic_local_array_lir_producer_path_no_clobber_certificate.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit LIR Producer Certificate Inputs

# Current Packet

## Just Finished

Closed idea 489 as a routed proof-population investigation and activated idea
490, `Dynamic Local-Array LIR Producer Path/No-Clobber Certificate`.

Reason:

- idea 489 confirmed `lir_producer_*` is a sufficient binding key and prepared
  branch/compare facts are candidate proof sources;
- proof population cannot proceed without a lower certificate for path coverage,
  dominance/guard validity, and dynamic-index same-value/no-clobber interval
  facts keyed to the same LIR producer site.

## Suggested Next

Execute Step 1: audit LIR producer certificate inputs. Determine whether
current prepared/BIR dominance, reachability, and effect/no-clobber surfaces can
produce a bounded certificate keyed to `lir_producer_*`.

## Watchouts

- Do not populate idea 486 checker inputs directly in this certificate packet.
- Do not infer path coverage or no-clobber from branch proximity, loop shape,
  value names, testcase names, dump order, final homes, or RV64 target behavior.
- Do not treat LIR producer instruction index as a prepared traversal/BIR
  instruction index without a separate conversion owner.
- Do not change idea 484 packaging, scalar local-load consumption, RV64/MIR
  lowering, implementation files, tests, review artifacts, or canonical logs
  until an executor receives a bounded packet.

## Proof

Lifecycle validation:

```sh
git diff --check
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_before.log --allow-non-decreasing-passed
python3 scripts/plan_review_state.py show
```

Result: passed.
