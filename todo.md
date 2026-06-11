Status: Active
Source Idea Path: ideas/open/179_bir_return_chain_consumer_migration.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add BIR Route Projection For AArch64

# Current Packet

## Just Finished

Step 2 - Add BIR Route Projection For AArch64 added an AArch64-local Route 8
return-chain projection/query adapter in
`src/backend/mir/aarch64/codegen/alu.cpp`.

The adapter builds a function-wide `bir::Route8ReturnChainIndex`, forms the
consumer-boundary key from `context.function.bir_function`,
`*context.bir_block`, `instruction_index`, the chain `bir::Value`, and the
chain `ValueNameId`, then returns fail-closed terminal or next-operand
`bir::Route1SourceValueIdentity` values only when the Route 8 record is
available. BIR APIs were sufficient, so `src/backend/bir/bir.hpp` and
`src/backend/bir/bir.cpp` were not changed.

The existing prepared terminal and next-operand semantic consumers remain in
place for Steps 3 and 4.

## Suggested Next

Execute Step 3 by migrating terminal return-chain recovery in
`find_return_chain_register`: thread or pass `binary->result` as the chain
value, read the Route 8 terminal identity through the new adapter, and keep the
existing AArch64 value-home, return ABI register, and retargeting policy local.

## Watchouts

- Keep AArch64 target policy out of BIR.
- Do not contract prepared return-chain APIs in this plan.
- Preserve fail-closed behavior for missing, invalid, or conflicting BIR route
  answers.
- `find_return_chain_register` still lacks the actual chain `bir::Value`; Step
  3 should use `binary->result` from `lower_scalar_instruction` or explicitly
  thread it into the helper.
- The adapter intentionally returns BIR identities rather than prepared
  `ValueNameId` answers; migration should map those identities through the
  existing AArch64 value-home/register machinery.
- The helper currently builds the Route 8 function index per query. Reuse can
  be introduced later if it becomes necessary, but it is not required for this
  minimal projection slice.

## Proof

Passed:

`(cmake --build build --target backend_aarch64_return_lowering_test backend_prepared_lookup_helper_test && ctest --test-dir build -R '^(backend_aarch64_return_lowering|backend_prepared_lookup_helper)$' --output-on-failure) > test_after.log 2>&1`

Proof log: `test_after.log`.
