Status: Active
Source Idea Path: ideas/open/369_semantic_bir_local_pointer_memory_observation_canonicalization.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair Pointer Local String-Literal Materialization

# Current Packet

## Just Finished

Step 2 repaired the string-literal pointer-local materialization path in
`try_lower_local_slot_store`. Stores fed by `global_pointer_slots` now keep the
lowered stored pointer value for the local store observation, while preserving
the existing AArch64 symbol-publication behavior and string materialization.

`pointer_param_loaded_char_deref` now emits the expected x86 semantic-BIR shape:
`%t0 = bir.load_local ptr %lv.p, addr .str0` followed by
`bir.store_local %lv.p, ptr %t0`.

## Suggested Next

Step 3 should repair the dynamic lane observation precedence around
`publish_dynamic_pointer_value_address` and the load/store ordering between
pointer-provenance and dynamic local/value-array handlers.

## Watchouts

This packet intentionally did not touch expectations, unsupported
classifications, runner/CTest contracts, target-machine backends, or Step 3
dynamic aggregate/member lane lowering. For Step 3, avoid a named-test fix: the
same precedence problem spans pointer-param, nested pointer-param, direct local,
store, load, and packed offset observations. Preserve generic computed-pointer
behavior for truly opaque pointer arithmetic.

## Proof

Proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_codegen_route_x86_64_pointer_param_loaded_char_deref_observe_semantic_bir|backend_lir_to_bir_notes|c_testsuite_aarch64_backend_src_00173_c)$' > test_after.log 2>&1`

Result: passed. `test_after.log` is the proof log. The subset reports
`backend_codegen_route_x86_64_pointer_param_loaded_char_deref_observe_semantic_bir`,
`backend_lir_to_bir_notes`, and `c_testsuite_aarch64_backend_src_00173_c` all
passed.
