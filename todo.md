Status: Active
Source Idea Path: ideas/open/369_semantic_bir_local_pointer_memory_observation_canonicalization.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Repair Dynamic Local Aggregate/Member Lane Materialization

# Current Packet

## Just Finished

Step 3 repaired dynamic local aggregate/member lane materialization precedence
in `lower_memory_load_inst` and `lower_memory_store_inst`. Dynamic
pointer-value arrays, dynamic local aggregate arrays, and dynamic scalar arrays
now get first refusal before generic pointer-provenance lowering when their
lane type matches the requested access type, so the six dynamic backend-route
targets lower through semantic-BIR lane loads/stores instead of opaque computed
pointer accesses.

The repair preserves generic computed-pointer behavior for casted or otherwise
wider/narrower accesses by letting scalar type mismatches fall through to the
pointer-provenance path.

## Suggested Next

Supervisor should review and commit this completed Step 3 slice, then choose
the next packet from the active plan.

## Watchouts

No expectations, unsupported classifications, runner/CTest contracts,
target-machine backend code, ABI/runtime buckets, or timeout buckets were
touched. The compatibility guard is type-based, not test-name, member-index,
packed-offset, temporary-name, or dump-line based.

## Proof

Proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_codegen_route_x86_64_pointer_param_loaded_char_deref_observe_semantic_bir|backend_codegen_route_x86_64_nested_pointer_param_dynamic_member_array_load_observe_semantic_bir|backend_codegen_route_x86_64_local_dynamic_member_array_observe_semantic_bir|backend_codegen_route_x86_64_local_dynamic_member_array_store_observe_semantic_bir|backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir|backend_codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir|backend_codegen_route_x86_64_packed_local_member_offsets_observe_semantic_bir|backend_lir_to_bir_notes|c_testsuite_aarch64_backend_src_00173_c)$' > test_after.log 2>&1`

Result: passed. `test_after.log` is the proof log. The subset reports all nine
delegated tests passed, including the six dynamic lane backend-route targets,
the Step 2 pointer-local target, `backend_lir_to_bir_notes`, and
`c_testsuite_aarch64_backend_src_00173_c`.
