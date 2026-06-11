Status: Active
Source Idea Path: ideas/open/179_bir_return_chain_consumer_migration.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Migrate Terminal Return-Chain Recovery

# Current Packet

## Just Finished

Step 3 - Migrate Terminal Return-Chain Recovery changed
`find_return_chain_register` in
`src/backend/mir/aarch64/codegen/alu.cpp` to recover terminal return-chain
identity from the AArch64 Route 8 adapter instead of
`prepare::find_prepared_return_chain_terminal_value`.

`lower_scalar_instruction` now passes `binary->result` as the chain BIR value.
The Route 8 terminal identity is resolved back to the existing prepared
value-home index, then mapped through the existing AArch64 before-return ABI
register lookup and retargeted to the chain result home as before. The adapter
keeps the prepared-name keyed query first and retries pure BIR identity only
for Route 8 `NoMatch`, because function-built Route 8 records currently carry
BIR names but not prepared name ids.

## Suggested Next

Execute Step 4 by migrating the next-operand rematerialized-immediate collision
check from `prepare::find_prepared_return_chain_next_operand_value` to the
Route 8 next-operand identity adapter while preserving the existing AArch64
scratch-register policy.

## Watchouts

- Keep AArch64 target policy out of BIR.
- Do not contract prepared return-chain APIs in this plan.
- Preserve fail-closed behavior for missing, invalid, or conflicting BIR route
  answers.
- The next-operand prepared read remains intentionally in place for Step 4.
- Route 8 records built directly from BIR do not carry prepared name ids, so
  the AArch64 adapter currently uses a `NoMatch` fallback to pure BIR identity.
- The helper still builds the Route 8 function index per query. Reuse can be
  introduced later if it becomes necessary, but it is not required for this
  migration slice.

## Proof

Passed:

`(cmake --build build --target backend_aarch64_return_lowering_test backend_prepared_lookup_helper_test && ctest --test-dir build -R '^(backend_aarch64_return_lowering|backend_prepared_lookup_helper)$' --output-on-failure) > test_after.log 2>&1`

Proof log: `test_after.log`.
