Status: Active
Source Idea Path: ideas/open/51_aarch64_alu_prepared_authority_repair.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Repair Select-Chain Scalar Publication Authority

# Current Packet

## Just Finished

Step 5: Repair Select-Chain Scalar Publication Authority completed.

Added a narrow shared prepared scalar select-chain materialization query that
packages the direct-global select-chain dependency with the authoritative root
value name for non-edge/non-store scalar consumers.

Routed `lower_scalar_select_publication` through that shared authority instead
of making the direct-global materialization decision locally in ALU. Existing
select-chain spelling and publication register behavior are preserved.

Added focused `backend_prepared_lookup_helper` coverage for the scalar
select-chain materialization query, including fail-closed missing-root
behavior.

## Suggested Next

Proceed to the next supervisor-selected packet from the active plan.

## Watchouts

Direct-global select-chain scalar publication now consumes shared
`PreparedScalarSelectChainMaterialization` authority. Store-source and
call-argument select-chain users still consume their existing shared prepared
dependency paths.

## Proof

Ran the delegated proof command exactly:

`bash -lc 'set -o pipefail; cmake --build --preset default 2>&1 | tee test_after.log; ctest --test-dir build -j --output-on-failure -R "^(backend_codegen_route_aarch64_global_function_pointer_table_selected_indirect_call|backend_aarch64_scalar_alu_records|backend_aarch64_prepared_scalar_alu_records|backend_aarch64_scalar_record_contract|backend_prepared_lookup_helper|backend_publication_plan_record|backend_store_source_publication_plan|c_testsuite_aarch64_backend_src_00164_c)$" 2>&1 | tee -a test_after.log'`

Result: build succeeded; all 8 delegated tests passed. Proof log:
`test_after.log`.
