Status: Active
Source Idea Path: ideas/open/57_aarch64_hfa_aggregate_return_result_consumption_repair.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Expose The Prepared HFA Result Lane Authority

# Current Packet

## Just Finished

Step 2 - Expose The Prepared HFA Result Lane Authority: added structured
`PreparedAfterCallResultLaneBinding` lookup state under
`PreparedMoveBundleLookups`. `make_prepared_function_lookups` now indexes
register-backed `AfterCall` `CallResultAbi` bindings by call block/index plus
the prepared destination result/lane value id, preserving the authoritative
move-bundle binding pointer, lane index, prepared value id/name, and call
position. AArch64 lowering can query it through
`module::find_prepared_after_call_result_lane_binding(context, block_index,
instruction_index, value_id)` or the underlying
`find_indexed_prepared_after_call_result_lane_binding` helper without raw BIR
scans or testcase-shaped matching.

## Suggested Next

Start Step 3: consume the prepared after-call result lane binding from the
AArch64 call-result scalar-state publication path so store-local lowering can
retarget HFA result-lane stores from stale locals to the authoritative `s0`/`s1`
call-result registers.

## Watchouts

- Treat `00204`, `ret`, `fr_hfa12`, `s9`, and `s13` as diagnostics only.
- Do not route implementation through idea 56 edge/terminator preservation
  while the first bad fact is HFA aggregate return-result consumption.
- Do not change variadic va_list layout, by-value argument publication, or
  callee-side return publication unless tracing proves they are the active
  first bad fact.
- Do not weaken expectations or mark supported probes unsupported.
- `PreparedCallPlan::result` for the HFA12 calls still shows
  `source_storage=none`; the new Step 2 lookup is the structured bridge to the
  existing `AfterCall` move-bundle lane facts. The next packet should consume
  that bridge rather than adding raw BIR scans or testcase-specific source
  selection.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_codegen_route_aarch64_pointer_select_aggregate_byte_copy|backend_codegen_route_aarch64_variadic_aggregate_overflow_byte_copy|backend_codegen_route_aarch64_alu_unpublished_load_local_(after_call|call_boundary)|c_testsuite_aarch64_backend_src_00164_c|c_testsuite_aarch64_backend_src_00176_c|c_testsuite_aarch64_backend_src_00181_c|c_testsuite_aarch64_backend_src_00204_c)$'`
was run and captured in `test_after.log`. Build succeeded; 7/8 focused tests
passed. `c_testsuite_aarch64_backend_src_00204_c` still fails with the known
runtime mismatch because this packet exposes the prepared authority but does
not yet consume it in the call-result publication path.
