Status: Active
Source Idea Path: ideas/open/179_bir_return_chain_consumer_migration.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Acceptance Validation

# Current Packet

## Just Finished

Step 6 - Acceptance Validation validated the AArch64 return-chain consumer
migration slice.

The focused AArch64 return lowering and Route 8 oracle tests are green. `rg`
confirms `src/backend/mir/aarch64/codegen/alu.cpp` has no
`find_prepared_return_chain_terminal_value`,
`find_prepared_return_chain_next_operand_value`, or `prepared_return_chain`
references, so the AArch64 consumer no longer reads the prepared return-chain
helper facts. The prepared return-chain lookup APIs remain public and
unchanged in `src/backend/prealloc/prepared_lookups.hpp` and are still covered
by `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`.

## Suggested Next

Supervisor should hand off to plan-owner for lifecycle review. The next
lifecycle prerequisite after this migration is
`ideas/open/180_bir_return_chain_prepared_api_contraction.md`.

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
- Do not contract prepared return-chain APIs until
  `ideas/open/180_bir_return_chain_prepared_api_contraction.md` is active.

## Proof

Passed:

`(cmake --build build --target backend_aarch64_return_lowering_test backend_prepared_lookup_helper_test && ctest --test-dir build -R '^(backend_aarch64_return_lowering|backend_prepared_lookup_helper)$' --output-on-failure) > test_after.log 2>&1`

Proof log: `test_after.log`.
