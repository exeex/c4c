Status: Active
Source Idea Path: ideas/open/179_bir_return_chain_consumer_migration.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Bound Or Remove Prepared Fallbacks

# Current Packet

## Just Finished

Step 5 - Bound Or Remove Prepared Fallbacks verified that
`src/backend/mir/aarch64/codegen/alu.cpp` has no remaining semantic calls to
`find_prepared_return_chain_terminal_value` or
`find_prepared_return_chain_next_operand_value`.

The remaining prepared access in the Route 8 AArch64 return-chain helper is
documented and bounded to translating an already-selected BIR Route 8 identity
back into the existing target value-home table; it does not consult prepared
return-chain fallback facts or contract the public prepared APIs.

## Suggested Next

Supervisor should decide whether the active runbook is exhausted enough for
plan-owner review/closure or whether a broader validation packet is needed
before closure.

## Watchouts

- Keep AArch64 target policy out of BIR.
- Do not contract prepared return-chain APIs in this plan.
- Preserve fail-closed behavior for missing, invalid, or conflicting BIR route
  answers.
- Route 8 records built directly from BIR do not carry prepared name ids, so
  the AArch64 adapter currently uses a `NoMatch` fallback to pure BIR identity.
- The helper still builds the Route 8 function index per query. Reuse can be
  introduced later if it becomes necessary, but it is not required for this
  migration slice.
- `rg` shows no `find_prepared_return_chain_terminal_value`,
  `find_prepared_return_chain_next_operand_value`, or `prepared_return_chain`
  reference remains in `src/backend/mir/aarch64/codegen/alu.cpp`.
- The Step 5 code change is intentionally documentation/bounding only because
  the remaining prepared lookup is a value-home bridge needed by target
  register selection, not a semantic return-chain source.

## Proof

Passed:

`(cmake --build build --target backend_aarch64_return_lowering_test backend_prepared_lookup_helper_test && ctest --test-dir build -R '^(backend_aarch64_return_lowering|backend_prepared_lookup_helper)$' --output-on-failure) > test_after.log 2>&1`

Proof log: `test_after.log`.
