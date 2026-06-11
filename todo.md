Status: Active
Source Idea Path: ideas/open/178_bir_return_chain_oracle_equivalence.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Add Rejected and Ambiguous Fail-Closed Equivalence Coverage

# Current Packet

## Just Finished

Step 4 rejected and ambiguous fail-closed equivalence coverage completed in
`tests/backend/bir/backend_prepared_lookup_helper_test.cpp`.
`verify_bir_return_chain_schema_and_index_lookup()` now compares Route 8 and
prepared return-chain helper fail-closed behavior for unsupported opcodes,
unnamed chain values, unnamed terminal return values, broken same-block walks,
non-return terminators, cross-block return relationships, and missing
instruction keys. Invalid fixtures assert that both terminal and next-operand
queries produce no usable answer.

## Suggested Next

Supervisor should choose the next coherent packet from the active plan, likely
Step 5 duplicate-conflict equivalence coverage.

## Watchouts

- Duplicate-conflict coverage remains for Step 5; this packet kept the
  existing Route 8 duplicate-only assertion unchanged.
- Prepared return-chain helpers stayed public and unchanged; no AArch64
  consumer migration was attempted.

## Proof

Delegated proof passed:
`(cmake --build build --target backend_prepared_lookup_helper_test && ctest --test-dir build -R '^backend_prepared_lookup_helper$' --output-on-failure) > test_after.log 2>&1`

Proof log: `test_after.log`.
