Status: Active
Source Idea Path: ideas/open/179_bir_return_chain_consumer_migration.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Migrate Next-Operand Alias And Scratch Recovery

# Current Packet

## Just Finished

Step 4 - Migrate Next-Operand Alias And Scratch Recovery changed the
rematerialized-immediate return-chain collision check in
`src/backend/mir/aarch64/codegen/alu.cpp` to recover the next operand through
the AArch64 Route 8 adapter instead of
`prepare::find_prepared_return_chain_next_operand_value`.

`lower_scalar_instruction` now passes `binary->result` as the chain BIR value
for next-operand identity lookup, resolves the Route 8 identity back through
the existing AArch64 value-home lookup, and preserves the target-local register
parsing, alias comparison, scalar view, scratch selection, and ALU record
construction.

## Suggested Next

Execute Step 5 by bounding or removing any remaining prepared return-chain
fallback/read dependency from migrated AArch64 consumers without contracting the
prepared API.

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
- `rg` shows no `find_prepared_return_chain_next_operand_value` call remains in
  `src/backend/mir/aarch64/codegen/alu.cpp`.

## Proof

Passed:

`(cmake --build build --target backend_aarch64_return_lowering_test backend_prepared_lookup_helper_test && ctest --test-dir build -R '^(backend_aarch64_return_lowering|backend_prepared_lookup_helper)$' --output-on-failure) > test_after.log 2>&1`

Proof log: `test_after.log`.
