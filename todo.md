Status: Active
Source Idea Path: ideas/open/488_bir_dynamic_local_array_consumer_coordinate_prepared_exposure.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Consumer Coordinate Exposure Inputs

# Current Packet

## Just Finished

Closed idea 487 as a routed blocker and activated prerequisite idea 488, `BIR
Dynamic Local-Array Consumer Coordinate Prepared Exposure`.

Reason:

- idea 487 proved real proof-source/path/no-clobber population cannot proceed
  without a durable consumer coordinate/prepared-exposure carrier;
- current facts can identify dynamic local-array element paths and prepared
  branch/compare facts separately, but cannot bind one proof source to one
  consumer interval without raw-shape/name/proximity inference.

Key evidence:

- `build/agent_state/487_step3_route_consumer_coordinate_prerequisite/route.md`
- `build/agent_state/487_step4_residual_disposition/disposition.md`

## Suggested Next

Execute Step 1: audit consumer coordinate exposure inputs. Determine whether
current BIR local-array element-path records, prepared traversal, and prepared
control-flow lookup surfaces can expose stable consumer coordinates and lookup
keys for dynamic local-array consumers.

## Watchouts

- Do not populate proof-source, path/dominance, or no-clobber facts in this
  coordinate exposure packet.
- Do not infer coordinates from dump order, testcase names, value names, branch
  proximity, loop shape, final homes, or RV64 target behavior.
- Do not mark dynamic range proofs available, change idea 486 checker
  vocabulary, package idea 484 records, consume scalar local loads, or change
  RV64/MIR lowering.
- Do not touch `review/`, canonical logs, baseline files, implementation files,
  or tests until an executor receives a bounded packet.

## Proof

Lifecycle validation:

```sh
git diff --check
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_before.log --allow-non-decreasing-passed
python3 scripts/plan_review_state.py show
```

Result: passed.
