Status: Active
Source Idea Path: ideas/open/375_aarch64_local_aggregate_bitfield_layout_publication.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Repair Layout/Store Publication

# Current Packet

## Just Finished

Step 3 repaired local aggregate member/bit-field GEP lowering for semantic BIR
by preserving aggregate byte-storage provenance when a local aggregate subobject
is represented through a byte leaf slot. A follow-up regression fix narrowed the
array-base dispatch diversion to byte-storage aggregate views only, so normal
`i32` nested array-field memcpy/memset paths keep their existing semantic BIR
lowering. The `00218` AArch64 local store publishes the bit-field storage unit
at layout byte offset `16` instead of logical index `2`, and the focused route
test still observes offset `16`.

## Suggested Next

Supervisor should review and commit this Step 3 slice, then decide whether the
active plan has remaining validation or lifecycle work.

## Watchouts

The repair is semantic/general: it does not special-case `00218`, field names,
registers, stack offsets, or emitted instructions. The array-base dispatch
ordering is intentionally changed only for byte leaf arrays carrying aggregate
subobject provenance; non-byte scalar array fields must continue using the
array-base path for builtin memcpy/memset lowering.

## Proof

Ran delegated proof:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_codegen_route_x86_64_local_aggregate_bitfield_layout_observe_semantic_bir|c_testsuite_aarch64_backend_src_00218_c|backend_codegen_route_x86_64_builtin_memcpy_nested_i32_array_field_observe_semantic_bir|backend_codegen_route_x86_64_builtin_memcpy_prefix_local_i32_array_to_pair_observe_semantic_bir|backend_codegen_route_x86_64_builtin_memset_nested_i32_array_field_observe_semantic_bir|backend_codegen_route_x86_64_builtin_memset_nonzero_nested_i32_array_field_observe_semantic_bir)$'; } > test_after.log 2>&1`.
Result: build succeeded; all six tests passed:
`backend_codegen_route_x86_64_local_aggregate_bitfield_layout_observe_semantic_bir`
and `c_testsuite_aarch64_backend_src_00218_c`, plus the four supervisor-reported
nested array-field memcpy/memset regression tests.
Proof log: `test_after.log`.
