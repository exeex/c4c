Status: Active
Source Idea Path: ideas/open/180_bir_return_chain_prepared_api_contraction.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Contract Eligible Prepared API

# Current Packet

## Just Finished

Plan Step 3 - Contract Eligible Prepared API is complete. The public prepared
return-chain semantic API was removed from
`src/backend/prealloc/prepared_lookups.hpp` without changing Route 8
return-chain semantics.

Contraction performed:

- Removed public `PreparedReturnChainLookups`, including
  `terminal_return_values_by_chain_value` and
  `next_operand_values_by_chain_value`.
- Removed `PreparedFunctionLookups::return_chains` and stopped
  `make_prepared_function_lookups` from constructing a prepared return-chain
  aggregate.
- Removed public `prepared_return_chain_value_key`,
  `make_prepared_return_chain_lookups`,
  `find_prepared_return_chain_terminal_value`, and
  `find_prepared_return_chain_next_operand_value`.
- Deleted the now-unreferenced prepared return-chain implementation helpers in
  `src/backend/prealloc/prepared_lookups.cpp`; no private prepared
  return-chain construction/query helper was retained because no production
  live dependency remained.
- Retargeted `backend_prepared_lookup_helper_test` semantic and fail-closed
  assertions to Route 8 only, including duplicate-conflict and
  missing-instruction cases, so the test no longer preserves public prepared
  helper access as an oracle.

## Suggested Next

Proceed to plan Step 4 with a post-contraction search/review packet that
confirms no public prepared return-chain semantic surface remains and prepares
the supervisor for broader acceptance or lifecycle review.

## Watchouts

- Text search after the edit found no remaining references in `src` or `tests`
  for `PreparedReturnChainLookups`, `.return_chains`,
  `prepared_return_chain_value_key`, `make_prepared_return_chain_lookups`,
  `find_prepared_return_chain_terminal_value`, or
  `find_prepared_return_chain_next_operand_value`.
- The delegated proof rebuilt AArch64 backend objects as part of the selected
  targets and did not expose an unexpected live AArch64 dependency.
- Route 8 assertions remain the semantic coverage for return-chain terminal,
  next-operand, fail-closed, duplicate-conflict, and missing-instruction
  behavior.

## Proof

Proof passed and is recorded in `test_after.log`.

```sh
(cmake --build build --target backend_aarch64_return_lowering_test backend_prepared_lookup_helper_test && ctest --test-dir build -R '^(backend_aarch64_return_lowering|backend_prepared_lookup_helper)$' --output-on-failure) > test_after.log 2>&1
```

Result: build succeeded; `backend_aarch64_return_lowering` and
`backend_prepared_lookup_helper` passed.
