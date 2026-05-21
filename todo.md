Status: Active
Source Idea Path: ideas/open/369_semantic_bir_local_pointer_memory_observation_canonicalization.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize the Semantic Observation Boundary

# Current Packet

## Just Finished

Step 1 localized the semantic observation boundary for the seven idea 369
backend-route residuals without code/test edits.

String-literal pointer-local target:

- Expected for
  `pointer_param_loaded_char_deref`: `bir.store_local %lv.p, ptr %t0`.
- Actual in `build/backend_route/pointer_param_loaded_char_deref_semantic_bir_x86_64.ll`:
  `@.str0 = bir.load_local ptr %lv.p, addr .str0` followed by
  `bir.store_local %lv.p, ptr @.str0`.
- First boundary/helper family: local scalar pointer store lowering in
  `lower_memory_store_inst` -> `try_lower_local_slot_store` when the stored
  value resolves through `global_pointer_slots`; the precise materialization
  helpers are `symbol_pointer_value_for_global_address` and
  `append_string_pointer_value_materialization`. That branch substitutes the
  string-symbol pointer value for the LIR temporary before the semantic dump.

Dynamic aggregate/member lane targets:

- Expected for pointer-param array lane load/store:
  `local_dynamic_member_array` wants
  `%t3.elt0 = bir.load_local i32 %t3.elt0.addr, addr %p.p`;
  `local_dynamic_member_array_store` wants
  `%t2.elt0 = bir.load_local i32 %t2.elt0.load.addr, addr %p.p`.
- Expected for nested pointer-param member array:
  `nested_pointer_param_dynamic_member_array_load` wants
  `%t5.elt0 = bir.load_local i32 %t5.elt0.addr, addr %t1+4`.
- Expected for direct local and packed local lanes:
  `local_direct_dynamic_member_array_load` wants
  `%t15.elt0 = bir.load_local i32 %lv.p.4`;
  `local_direct_dynamic_member_array_store` wants
  `%t12.elt0 = bir.load_local i32 %lv.p.0`;
  `packed_local_member_offsets` wants
  `%t15.elt0 = bir.load_local i32 %lv.p.1`.
- Actual semantic-BIR instead emits computed-address GEP shape before the
  load/store, for example `%t2/%t4/%t14 = bir.add ptr ... byte_offset` followed
  by `bir.load_local ... addr %t*` or `bir.store_local ... addr %t*`.
- First boundary/helper family: `lower_memory_gep_inst` in
  `memory/addressing.cpp` publishes dynamic local/member GEPs through
  `publish_dynamic_pointer_value_address`, which records both
  `pointer_value_addresses_` and `dynamic_pointer_value_arrays_`. The later
  `lower_memory_load_inst`/`lower_memory_store_inst` paths in
  `memory/local_slots.cpp` run pointer-provenance lowering
  (`try_lower_pointer_provenance_load`/`try_lower_pointer_provenance_store`)
  before the dynamic handlers (`load_dynamic_pointer_value_array_value`,
  `append_dynamic_pointer_value_array_store`,
  `try_lower_dynamic_local_aggregate_load`, and
  `try_lower_dynamic_local_aggregate_store`). That ordering causes the generic
  computed-pointer observation to win before the `*.elt*` lane helpers can
  produce canonical local/member-lane loads/stores.

## Suggested Next

Keep Steps 2 and 3 split. Step 2 should repair only the string-literal
pointer-local store materialization branch around
`symbol_pointer_value_for_global_address` /
`append_string_pointer_value_materialization`. Step 3 should repair the
dynamic lane observation precedence around `publish_dynamic_pointer_value_address`
and the load/store ordering between pointer-provenance and dynamic
local/value-array handlers.

## Watchouts

Do not treat these local backend-route snippet failures as AArch64 or x86
target-machine failures. Do not claim progress through expectation rewrites,
unsupported classifications, runner changes, timeout policy changes, CTest
registration changes, proof-log policy changes, or named-test special cases.
For Step 3, avoid a named-test fix: the same precedence problem spans
pointer-param, nested pointer-param, direct local, store, load, and packed
offset observations. Preserve generic computed-pointer behavior for truly
opaque pointer arithmetic.

## Proof

Proof command:
`test -s test_after.log && rg 'backend_codegen_route_x86_64_(pointer_param_loaded_char_deref|nested_pointer_param_dynamic_member_array_load|local_dynamic_member_array|local_dynamic_member_array_store|local_direct_dynamic_member_array_store|local_direct_dynamic_member_array_load|packed_local_member_offsets)_observe_semantic_bir' test_after.log >/dev/null`

Result: passed. This was an evidence-localization packet only; no build,
implementation, test, expectation, unsupported-classification, runner, CTest
contract, or `test_after.log` edits were made.
