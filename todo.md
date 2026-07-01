Status: Active
Source Idea Path: ideas/open/486_bir_index_range_proof_path_dominance_carrier.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Index Range Proof Inputs

# Current Packet

## Just Finished

Closed idea 485 as complete for source-object, derivation, and layout carrier
records and activated prerequisite idea 486, `BIR Index Range Proof And
Path-Dominance Carrier`.

Reason:

- idea 485 Step 3 added durable local-array source-object, derivation, and
  element-path/layout records;
- dynamic local-array GEP rows remain unavailable as
  `missing_index_range_proof`;
- range proof, path/dominance validity, and index no-clobber are separate
  proof-carrier responsibilities.

Key evidence:

- `build/agent_state/485_step3_local_array_carrier_producer/summary.md`
- `build/agent_state/485_step4_residual_disposition/disposition.md`

## Suggested Next

Execute Step 1: audit index range proof inputs for dynamic local-array indices.
Determine whether current control-flow/proof surfaces can provide explicit
proof source, bounds, path/dominance coverage, and no-clobber facts for a
representative dynamic local-array access.

## Watchouts

- Do not infer index range from loop shape, value names, testcase names, dump
  order, final homes, or RV64 target behavior.
- Do not mark dynamic GEP rows available without proof source,
  path/dominance, and no-clobber facts.
- Do not change idea 485 carrier records, idea 484 packaging, scalar local-load
  consumption, or RV64/MIR lowering in this proof-carrier packet.
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
