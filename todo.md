Status: Active
Source Idea Path: ideas/open/178_bir_return_chain_oracle_equivalence.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Positive Terminal Equivalence Coverage

# Current Packet

## Just Finished

Step 2 positive terminal-equivalence coverage completed in
`tests/backend/bir/backend_prepared_lookup_helper_test.cpp`.
`verify_bir_return_chain_schema_and_index_lookup()` now builds a matching
prepared return-chain lookup surface for the existing same-block `%seed` ->
`%ret` fixture and compares the prepared terminal/next helper answers against
the real Route 8 answers returned from
`route8_build_return_chain_index(function)`. Prepared answers are asserted to
have valid `ValueNameId`s, and Route 8 identity spellings are compared against
`prepared.names.value_names.spelling(...)` for `%seed`, `%named.next`, and the
terminal `%ret` no-next case.

## Suggested Next

Supervisor should review/commit the Step 2 coverage slice, then choose the next
packet from the active plan.

## Watchouts

- Route 8 automatic index construction does not carry prepared `ValueNameId`s;
  this coverage intentionally compares its normal accepted identity spelling
  against the prepared table spelling for each valid prepared answer.
- Prepared return-chain helpers stayed public and unchanged; no AArch64
  consumer migration was attempted.

## Proof

Delegated proof passed:
`(cmake --build build --target backend_prepared_lookup_helper_test && ctest --test-dir build -R '^backend_prepared_lookup_helper$' --output-on-failure) > test_after.log 2>&1`

Proof log: `test_after.log`.
