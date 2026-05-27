Status: Active
Source Idea Path: ideas/open/57_aarch64_hfa_aggregate_return_result_consumption_repair.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Reproduce And Trace The HFA Return-Result Consumer

# Current Packet

## Just Finished

Step 1 - Reproduce And Trace The HFA Return-Result Consumer: reproduced the
focused failure and traced the caller-side consumer after `bl fr_hfa12`.
Prepared BIR for `ret` has `%t95 = bir.call %struct.hfa12 fr_hfa12()` at
instruction 346, followed by `bir.store_local %t96.agg.0, float %t95` and
`bir.store_local %t96.agg.4, float %t95.hfa.ret.lane.1`; the second call at
instruction 351 mirrors this with `%t100` and `%t100.hfa.ret.lane.1`.
The emitted AArch64 for those store-local consumers is stale:
`bl fr_hfa12; str s9, [sp, #1084]; str s13, [sp, #1088]`, then the second call
stores `s9`/`s13` again before converting the stack reloads for `printf`.
The prepared `AfterCall` move bundles already contain the missing authority:
for instruction 346, value ids 1912/1913 have `abi_index=0/1` and
`fpr:call_result#0/#1` registers `s0`/`s1`; for instruction 351, value ids
1916/1917 have the same `s0`/`s1` lane facts. The smallest likely
implementation owner is the AArch64 call-result publication to store-local
handoff: `record_call_result_source_register` currently records only the
single `PreparedCallPlan::result` source, while HFA aggregate calls expose the
lane authority in `AfterCall` move-bundle `abi_binding`s; the stale consumer is
then reached through `dispatch.cpp` ordinary memory lowering,
`retarget_fpr_call_result_store_value_to_emitted_scalar`, and
`lower_store_local_value_publication`.

## Suggested Next

Start Step 2: expose or reuse a structured prepared lookup that maps the
post-call HFA result values `%t95`/`%t95.hfa.ret.lane.1` and
`%t100`/`%t100.hfa.ret.lane.1` to their `AfterCall` ABI bindings (`s0`/`s1`),
then make the AArch64 scalar-state publication path record those lane sources
before store-local lowering consumes them.

## Watchouts

- Treat `00204`, `ret`, `fr_hfa12`, `s9`, and `s13` as diagnostics only.
- Do not route implementation through idea 56 edge/terminator preservation
  while the first bad fact is HFA aggregate return-result consumption.
- Do not change variadic va_list layout, by-value argument publication, or
  callee-side return publication unless tracing proves they are the active
  first bad fact.
- Do not weaken expectations or mark supported probes unsupported.
- `PreparedCallPlan::result` for the HFA12 calls shows `source_storage=none`,
  but the matching `move_bundle phase=after_call` rows at instruction 346 and
  351 carry the authoritative `abi_binding` lane facts. Step 2 should prefer
  exposing those existing move-bundle facts over adding raw BIR scans or
  testcase-specific source selection.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_codegen_route_aarch64_pointer_select_aggregate_byte_copy|backend_codegen_route_aarch64_variadic_aggregate_overflow_byte_copy|backend_codegen_route_aarch64_alu_unpublished_load_local_(after_call|call_boundary)|c_testsuite_aarch64_backend_src_00164_c|c_testsuite_aarch64_backend_src_00176_c|c_testsuite_aarch64_backend_src_00181_c|c_testsuite_aarch64_backend_src_00204_c)$'`
was run and captured in `test_after.log`. Build was up to date; 7/8 focused
tests passed; `c_testsuite_aarch64_backend_src_00204_c` still fails with the
known runtime mismatch, preserving the Step 1 reproduction evidence.
