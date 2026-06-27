Status: Active
Source Idea Path: ideas/open/417_prepared_storage_layout_and_initializer_contracts.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Add Prepared Fact And Verifier Classification Support

# Current Packet

## Just Finished

Step 3 added prepared verifier support for the selected local storage and
global/object-data contract slice from
`docs/prepared_fact_contracts/storage_initializer_contract_plan.md`.

`PreparedContractFactFamily` now names the selected storage extent, alignment,
byte-range, alias-authority, memory-provenance, object-label,
publication-identity, emitted-byte, zero-fill, relocation, object-byte-range,
and unsupported-object-data-marker families. The verifier now exposes compact
selected local-storage and object-data fact structures plus classification and
report functions.

Focused tests cover coherent selected facts, missing selected facts as
`producer_missing`, contradictory facts as `producer_incoherent`, complete
unsupported forms as `target_unsupported_but_coherent`, and invalid
pre-prepared initializer semantics as `pre_prepared_semantic_failure`.

## Suggested Next

Execute Step 4 by wiring the selected verifier facts into the first producer or
consumer boundary chosen by the supervisor, without crossing the Idea 415 raw
symbol spelling/value-name normalization boundary.

## Watchouts

- Step 3 intentionally stopped at verifier/API and focused tests; it did not
  migrate RV64 target consumers or populate object-data producers.
- The selected object-data verifier facts keep raw symbol spelling and
  value-name normalization out of the contract payload, preserving the Idea 415
  boundary.
- Step 4 should decide the first concrete producer/consumer integration point
  and populate these contract facts there rather than adding target-side
  reconstruction.

## Proof

Ran exactly:
`( cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prealloc_|backend_prepared_object_consumer_contract$|backend_riscv_object_emission$)' ) > test_after.log 2>&1`

Result: passed. `test_after.log` records 8/8 selected tests passing, including
`backend_prealloc_prepared_contract_verifier`,
`backend_prepared_object_consumer_contract`, and
`backend_riscv_object_emission`.
