Status: Active
Source Idea Path: ideas/open/185_phase_e_route2_select_chain_view_consumer_migration.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Expose Or Reuse A Narrow Route 2 View

# Current Packet

## Just Finished

Step 2 of `plan.md` exposed the selected AArch64 ALU control-publication
select consumer to the existing Route 2 select-chain identity view. Added
`route2_control_select_chain_materialization(...)` in
`src/backend/mir/aarch64/codegen/alu.cpp`, keyed by `select.result` and
`instruction_index + 1U`, requiring a select root, root instruction index, and
scalar materialization eligibility before mapping only root identity and
direct-global presence into the existing prepared-shaped materialization data.

The selected consumer now tries that Route 2 view first and keeps
`prepare::find_prepared_scalar_select_chain_materialization(...)` as the
unchanged fallback/oracle when Route 2 has no valid answer. Result-register
selection, stack publication, materialization sequencing, final record spelling,
memory/call policy, and storage/home decisions stayed outside Route 2.

## Suggested Next

Run the next supervisor-selected Step 3 packet against the same selected ALU
select-result consumer, likely tightening or proving the consumer-side
fallback/visibility contract without widening to call, memory, publication, or
aggregate Route 2 users.

## Watchouts

The adapter intentionally depends on the typed
`mir::BirSelectChainIdentityRequest` path and does not rescan broad BIR or
perform ad hoc name matching beyond the Route 2 request. Keep the prepared
select-chain/direct-global helpers public and unchanged as fallback/oracle.
Do not extend this packet into `dispatch_value_materialization.cpp`,
`dispatch_producers.cpp`, calls, memory/store consumers, publication planning,
printer/debug surfaces, or expectation rewrites.

## Proof

Passed the supervisor-selected proof:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepared_lookup_helper|backend_aarch64_prepared_scalar_alu_records|backend_aarch64_scalar_alu_records|backend_aarch64_scalar_record_contract)$'; } > test_after.log 2>&1`

Proof log: `test_after.log`.
