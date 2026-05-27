Status: Active
Source Idea Path: ideas/open/51_aarch64_alu_prepared_authority_repair.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Repair Return ABI And Return-Chain Authority

# Current Packet

## Just Finished

Step 4: Repair Return ABI And Return-Chain Authority completed.

Confirmed return ABI register selection already consumes
`find_prepared_before_return_abi_move_by_source_and_destination_bank`, and
return-chain terminal selection already consumes
`find_prepared_return_chain_terminal_value` plus indexed value-home lookup.

Removed the remaining ALU-local forward BIR name-chain lookahead used for
return-chain scratch selection. The next operand in a return chain is now
published through `PreparedReturnChainLookups` and consumed by ALU via the
indexed `find_prepared_return_chain_next_operand_value` query.

## Suggested Next

Proceed to the next supervisor-selected packet from the active plan.

## Watchouts

No ALU-local raw move-bundle scan remains for return ABI selection. The only
return-chain BIR walk is in shared prepared lookup construction; ALU consumes
the indexed prepared return-chain facts.

## Proof

Ran the delegated proof command exactly:

`bash -lc 'set -o pipefail; cmake --build --preset default 2>&1 | tee test_after.log; ctest --test-dir build -j --output-on-failure -R "^(backend_aarch64_return_lowering|backend_codegen_route_aarch64_alu_unpublished_load_local_after_call|backend_codegen_route_aarch64_alu_unpublished_load_local_call_boundary|backend_aarch64_scalar_alu_records|backend_aarch64_prepared_scalar_alu_records|backend_aarch64_scalar_record_contract|backend_prepared_lookup_helper|backend_cli_aarch64_asm_external_return_zero_smoke|backend_cli_aarch64_asm_external_return_add_smoke|backend_cli_aarch64_asm_external_return_add_sub_chain_smoke)$" 2>&1 | tee -a test_after.log'`

Result: build succeeded; all 10 delegated tests passed. Proof log:
`test_after.log`.
