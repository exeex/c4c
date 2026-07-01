Status: Active
Source Idea Path: ideas/open/489_bir_dynamic_local_array_proof_population_from_lir_coordinates.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit LIR Coordinate Proof Population Inputs

# Current Packet

## Just Finished

Closed idea 488 as the prerequisite LIR producer-coordinate exposure slice and
activated idea 489, `BIR Dynamic Local-Array Proof Population From LIR
Coordinates`.

Reason:

- Step 3 exposed truthful `LocalArrayElementPathRecord::lir_producer_*` fields
  and `lir-producer:` lookup keys for address-derivation paths.
- Step 4 says proof-source/path/no-clobber population should resume separately
  by consuming that LIR producer-coordinate surface.
- Dynamic rows remain fail-closed as `missing_index_range_proof` until those
  proof facts are populated and accepted by the idea 486 checker.

## Suggested Next

Execute Step 1: audit whether existing prepared branch/compare facts,
control-flow/path surfaces, no-clobber evidence, and dynamic local-array
`lir_producer_*` coordinates can support a bounded proof-population packet.

## Watchouts

- Do not treat `lir_producer_instruction_index` as a prepared traversal or BIR
  `Block::insts` instruction index.
- Do not infer proof facts from loop shape, branch proximity, value names,
  testcase names, dump order, final homes, or RV64 target behavior.
- Do not change idea 488 coordinate exposure, idea 486 checker vocabulary, idea
  484 packaging, scalar local-load consumption, or RV64/MIR lowering.
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
