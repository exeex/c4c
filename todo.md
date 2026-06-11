Status: Active
Source Idea Path: ideas/open/178_bir_return_chain_oracle_equivalence.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Add Positive Next-Operand Equivalence Coverage

# Current Packet

## Just Finished

Step 3 positive next-operand equivalence coverage completed in
`tests/backend/bir/backend_prepared_lookup_helper_test.cpp`.
`verify_bir_return_chain_schema_and_index_lookup()` now has an explicit
next-operand oracle-equivalence assertion for the existing same-block `%seed`
-> `%ret` fixture. The assertion compares the Route 8 immediate next operand
for `%seed` against
`find_prepared_return_chain_next_operand_value(...)`, and also checks that both
Route 8 and the prepared helper agree that terminal `%ret` has no next operand.

## Suggested Next

Supervisor should choose the next coherent packet from the active plan, likely
Step 4 rejected and ambiguous fail-closed equivalence coverage.

## Watchouts

- Route 8 automatic index construction still does not carry prepared
  `ValueNameId`s; this coverage compares normal Route 8 identity spellings
  against prepared table spellings for valid prepared answers.
- Prepared return-chain helpers stayed public and unchanged; no AArch64
  consumer migration was attempted.

## Proof

Delegated proof passed:
`(cmake --build build --target backend_prepared_lookup_helper_test && ctest --test-dir build -R '^backend_prepared_lookup_helper$' --output-on-failure) > test_after.log 2>&1`

Proof log: `test_after.log`.
