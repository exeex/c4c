Status: Active
Source Idea Path: ideas/open/54_aarch64_global_value_materialization_consumer_authority_repair.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Route FP LoadGlobal Materialization Through Prepared Authority

# Current Packet

## Just Finished

Completed Step 3 implementation for `fp_value_materialization.cpp` same-block
FP `LoadGlobal` materialization.

The FP materializer now resolves the named value through
`prepare::find_prepared_same_block_scalar_producer(...)`, then validates the
producer/access pair with
`prepare::find_prepared_same_block_global_load_access(...)`. When that prepared
global-load access is available, emission derives the symbol with
`prepare::prepared_link_name(...)`, derives direct/GOT eligibility with
`prepare::prepared_global_symbol_address_policy(access.address,
context.function.target_profile)`, and uses `access.address.byte_offset` for
the final load address. This prepared path emits the same instruction spelling
and destination/scratch register behavior as the previous FP path.

## Suggested Next

Delegate the next coherent Step 4 packet for the remaining global value
materialization consumer authority route.

## Watchouts

The legacy raw `find_load_global_target` / `load_global_symbol_label` fallback
remains only for cases where prepared global-load access is unavailable or
cannot emit a concrete direct/GOT policy. The prepared FP path is the authority
when the prepared fact can express the producer.

## Proof

Ran exactly:

`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_codegen_route_aarch64_scalar_fp_literal_add_publishes_fpr_result|backend_codegen_route_aarch64_global_function_pointer_table_selected_indirect_call|backend_cli_dump_prepared_bir_is_prepared|backend_cli_dump_prepared_bir_exposes_contract_sections)$') > test_after.log 2>&1`

Result: passed. `test_after.log` records a successful preset build and 4/4
matching tests passed.
