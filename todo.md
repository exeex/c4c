Status: Active
Source Idea Path: ideas/open/168_route2_select_chain_direct_global_oracle_thinning.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Validate And Handoff

# Current Packet

## Just Finished

Completed the scan portion of Step 4 of `plan.md`: re-scanned direct consumers
after the selected AArch64 scalar value-publication Route 2 migration and found
no prepared select-chain/direct-global/scalar-materialization surface that is
proven private enough to contract yet.

Consumer evidence:
- `find_prepared_select_chain_source_producer(...)` is still a production
  dependency in `src/backend/prealloc/publication_plans.cpp` and remains an
  internal dependency of prepared direct-global and scalar select-chain helper
  APIs.
- `find_prepared_direct_global_select_chain_dependency(...)` is still used by
  `src/backend/prealloc/call_plans.cpp`, the AArch64 indirect-callee path in
  `src/backend/mir/aarch64/codegen/calls.cpp`, and the prepared scalar
  materialization helper.
- `find_prepared_store_source_direct_global_select_chain_dependency(...)` is
  still used by prepared publication planning and AArch64 memory/store-source
  lowering.
- `find_prepared_scalar_select_chain_materialization(...)` is still used by
  `src/backend/mir/aarch64/codegen/alu.cpp` and prepared printer select-chain
  output.
- `PreparedFunctionLookups::edge_publication_source_producers` still has broad
  direct consumers across AArch64 producer, edge-copy, memory, FP materialization,
  comparison, ALU, and call codegen paths.
- `prepare::PreparedDirectGlobalSelectChainDependency` still crosses public
  APIs in call plans, select-chain materialization, AArch64 test fixtures, and
  the local adapter kept by the migrated value-publication consumer.

Route 2-backed users are not broad enough to replace those surfaces:
- `mir::find_bir_select_chain_identity(...)` is used by the selected migrated
  `dispatch_value_materialization.cpp` consumer and by oracle tests.
- `mir::find_bir_select_chain_direct_global_dependency(...)` is used by
  `dispatch_producers.cpp` plus oracle tests.
- `mir::find_bir_select_chain_source_producer(...)` and
  `mir::find_bir_select_chain_scalar_materialization_eligibility(...)` remain
  oracle-test-only outside their declarations/definitions.

## Suggested Next

Move to Step 5 validation and handoff for a no-contraction Route 2 result. Use
the existing narrow proof command:
`bash -lc "set -o pipefail; cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepared_lookup_helper|backend_aarch64_instruction_dispatch)$'" |& tee test_after.log`

## Watchouts

- No contraction is safe in this packet: the remaining consumers are production
  paths, not only tests or private implementation details.
- Keep `select_chain_lookups.hpp`,
  `PreparedFunctionLookups::edge_publication_source_producers`,
  `PreparedDirectGlobalSelectChainDependency`, and the prepared scalar
  select-chain materialization struct public until later Route 2 packets migrate
  call planning, store/publication planning, AArch64 ALU/calls/memory/FP
  consumers, and prepared printer expectations.
- The selected value-publication consumer still needs a prepared-shaped local
  adapter because `emit_select_chain_value_to_register(...)` accepts
  `prepare::PreparedDirectGlobalSelectChainDependency`; that is residual target
  API compatibility, not evidence that the semantic lookup migration failed.

## Proof

No build or test run; the delegated packet was scan-only and explicitly required
no build. No new proof log was produced and root-level log files were left
untouched.
