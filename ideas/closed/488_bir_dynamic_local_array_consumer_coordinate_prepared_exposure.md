# BIR Dynamic Local-Array Consumer Coordinate Prepared Exposure

Status: Closed
Type: BIR/prepared metadata exposure prerequisite idea
Parent: `ideas/closed/487_bir_dynamic_local_array_proof_source_path_no_clobber_population.md`
Source Evidence:
- `build/agent_state/487_step3_route_consumer_coordinate_prerequisite/route.md`
- `build/agent_state/487_step4_residual_disposition/disposition.md`
Owning Layer: BIR local-array carrier to prepared consumer-coordinate exposure
Closed By: lifecycle review after Step 4

## Completion Notes

Idea 488 is complete as the prerequisite LIR producer-coordinate exposure slice
for dynamic local-array address-derivation paths.

Step 3 exposed durable `LocalArrayElementPathRecord::lir_producer_*` metadata
and stable `lir-producer:` lookup keys for address-derivation paths. The
coordinate is explicitly a LIR producer-site coordinate, not a prepared object
traversal coordinate and not a BIR `Block::insts` instruction index.

Completed evidence:

- `build/agent_state/488_step1_consumer_coordinate_exposure_inputs/audit.md`
- `build/agent_state/488_step2_consumer_coordinate_exposure_contract/contract.md`
- `build/agent_state/488_step3_consumer_coordinate_exposure/summary.md`
- `build/agent_state/488_step4_residual_disposition/disposition.md`

## Handoff

The next active source is:

`ideas/open/489_bir_dynamic_local_array_proof_population_from_lir_coordinates.md`

That source should consume the committed `lir_producer_*` fields and
`lir-producer:` keys to populate proof-source/path/no-clobber facts. It should
not require idea 488 to add prepared traversal/BIR instruction coordinates.

## Residual Disposition

Remaining owners are separate from idea 488:

- proof-source record population for lower/upper bound branch or compare facts;
- path/dominance coverage from proof source to the LIR producer site;
- no-clobber/same-value interval facts for the dynamic index;
- load/store consumer use-linking beyond the address-derivation producer role;
- prepared traversal/BIR instruction coordinate conversion, if a later idea
  explicitly needs it;
- idea 484 local-address packaging;
- scalar local-load consumption;
- RV64/MIR lowering.

Dynamic rows remain fail-closed on range/path/no-clobber availability until
that separate proof population proves those facts.

## Validation

Lifecycle validation:

```sh
git diff --check
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_before.log --allow-non-decreasing-passed
```

Both passed. `test_after.log` was absent at close time, and this lifecycle task
forbade touching canonical logs, so no new after log was generated.

## Reviewer Reject Signals

Reject reopening this source as progress if the change:

- treats `lir_producer_instruction_index` as a prepared traversal or BIR
  `Block::insts` instruction index without an explicit conversion owner;
- populates proof-source, path/dominance, or no-clobber facts inside this
  closed coordinate exposure slice;
- marks dynamic range proofs available, changes idea 486 checker vocabulary,
  packages idea 484 records, consumes scalar local loads, or changes RV64/MIR
  lowering;
- infers coordinates from dump order, testcase names, value names, branch
  proximity, loop shape, final homes, or target behavior;
- changes expectations, unsupported markers, allowlists, pass/fail accounting,
  runtime comparison, or baseline logs as proof of progress.
