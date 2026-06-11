Status: Active
Source Idea Path: ideas/open/180_bir_return_chain_prepared_api_contraction.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Reconfirm Prerequisite Proof

# Current Packet

## Just Finished

Plan Step 1 - Reconfirm Prerequisite Proof is complete. The focused
prepared/BIR lookup helper and AArch64 return lowering prerequisite proof is
green, and `src/backend/mir/aarch64/codegen/alu.cpp` has no references to
`find_prepared_return_chain_terminal_value`,
`find_prepared_return_chain_next_operand_value`, or `prepared_return_chain`.

## Suggested Next

Proceed to plan Step 2 by contracting the public prepared return-chain API in
`src/backend/prealloc/prepared_lookups.hpp` and matching implementation exports
in `src/backend/prealloc/prepared_lookups.cpp`.

## Watchouts

- Step 2 public prepared API inventory: `PreparedReturnChainLookups`,
  `PreparedFunctionLookups::return_chains`, `prepared_return_chain_value_key`,
  `make_prepared_return_chain_lookups`,
  `find_prepared_return_chain_terminal_value`, and
  `find_prepared_return_chain_next_operand_value` remain public in
  `prepared_lookups.hpp`.
- Current non-implementation users of the return-chain helper symbols are in
  `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`; the implementation
  also builds `PreparedFunctionLookups::return_chains` internally in
  `make_prepared_function_lookups`.
- Do not weaken tests or downgrade supported-path expectations.
- Keep API cleanup separate from new BIR schema behavior or consumer migration.

## Proof

Passed:

```sh
(cmake --build build --target backend_aarch64_return_lowering_test backend_prepared_lookup_helper_test && ctest --test-dir build -R '^(backend_aarch64_return_lowering|backend_prepared_lookup_helper)$' --output-on-failure) > test_after.log 2>&1
```

`test_after.log` contains the fresh run: both
`backend_aarch64_return_lowering` and `backend_prepared_lookup_helper` passed.
