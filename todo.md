Status: Active
Source Idea Path: ideas/open/63_aarch64_shared_select_chain_dependency_authority.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Define Shared Select-Chain Dependency Facts

# Current Packet

## Just Finished

Completed Step 2 coherent shared fact slice for call arguments. Added
`PreparedCallArgumentDirectGlobalSelectChainDependency` on
`PreparedCallArgumentPlan`, with
`find_prepared_call_argument_direct_global_select_chain_dependency` as the
fail-closed query surface. `populate_call_plans` now fills that fact from the
existing prepared edge publication source-producer authority and
`find_prepared_direct_global_select_chain_dependency`, preserving root value
identity, direct-global presence, root-is-select, and root instruction index.

Coverage added:
- `backend_prepared_lookup_helper_test` covers the new call-argument query and
  fail-closed behavior for a missing fact.
- `backend_prepare_frame_stack_call_contract_test` prepares a real call whose
  argument is a select chain over a direct global load and verifies the
  populated call argument plan carries the shared dependency authority.

## Suggested Next

Supervisor should delegate Step 3 to migrate
`materialize_direct_global_select_chain_call_argument` so it consumes
`PreparedCallArgumentPlan` via
`find_prepared_call_argument_direct_global_select_chain_dependency` instead of
calling `publication_plans` directly.

## Watchouts

This packet intentionally did not migrate any AArch64 consumer. Remaining
shared fact gaps from Step 1 still stand for generic value publication,
indirect callees, and the AArch64-local `dispatch_producers.cpp` direct-global
traversal/raw BIR fallback. `PreparedScalarSelectChainMaterialization` still
covers the ALU direct-global case and should not be duplicated.

## Proof

Proof command run exactly:
`(cmake --build build -j && ctest --test-dir build -j --output-on-failure -R 'backend_(prepared_lookup_helper|prepare_frame_stack_call_contract|prealloc_call_boundary_classification|prepared_printer)') > test_after.log 2>&1`

Result: passed; `test_after.log` is the canonical proof log. `git diff --check`
also passed.
