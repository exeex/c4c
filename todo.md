Status: Active
Source Idea Path: ideas/open/295_backend_regex_failure_family_inventory.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Reconstruct the Current Backend Regex Surface

# Current Packet

## Just Finished

Step 1 - Reconstruct the Current Backend Regex Surface: captured the current
backend regex surface after idea 368 closed using `test_after.log`.

Log source:
`cmake --build --preset default && ctest --test-dir build -j10 -R backend --output-on-failure > test_after.log 2>&1`

Counts from `test_after.log`:
- selected: 355
- passed: 329
- failed: 24
- timed out: 2
- non-passing total: 26

Current non-passing local backend bucket:
- `backend_codegen_route_x86_64_pointer_param_loaded_char_deref_observe_semantic_bir`
- `backend_codegen_route_x86_64_nested_pointer_param_dynamic_member_array_load_observe_semantic_bir`
- `backend_codegen_route_x86_64_local_dynamic_member_array_observe_semantic_bir`
- `backend_codegen_route_x86_64_local_dynamic_member_array_store_observe_semantic_bir`
- `backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir`
- `backend_codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir`
- `backend_codegen_route_x86_64_packed_local_member_offsets_observe_semantic_bir`

Current non-passing external `c_testsuite_aarch64_backend_*` bucket:
- `c_testsuite_aarch64_backend_src_00112_c`
- `c_testsuite_aarch64_backend_src_00119_c`
- `c_testsuite_aarch64_backend_src_00123_c`
- `c_testsuite_aarch64_backend_src_00140_c`
- `c_testsuite_aarch64_backend_src_00143_c`
- `c_testsuite_aarch64_backend_src_00157_c`
- `c_testsuite_aarch64_backend_src_00163_c`
- `c_testsuite_aarch64_backend_src_00174_c`
- `c_testsuite_aarch64_backend_src_00176_c`
- `c_testsuite_aarch64_backend_src_00182_c`
- `c_testsuite_aarch64_backend_src_00183_c`
- `c_testsuite_aarch64_backend_src_00185_c`
- `c_testsuite_aarch64_backend_src_00187_c`
- `c_testsuite_aarch64_backend_src_00200_c` (Timeout)
- `c_testsuite_aarch64_backend_src_00205_c`
- `c_testsuite_aarch64_backend_src_00207_c` (Timeout)
- `c_testsuite_aarch64_backend_src_00216_c`
- `c_testsuite_aarch64_backend_src_00218_c`
- `c_testsuite_aarch64_backend_src_00204_c`

## Suggested Next

Use this captured surface as the Step 1 inventory baseline, then choose the
next classification packet from the local backend bucket or external
`c_testsuite_aarch64_backend_*` bucket.

## Watchouts

No semantic owner classification was attempted in this packet. The external
bucket contains 17 failed tests and 2 timed-out tests; the local backend bucket
contains 7 failed tests.

## Proof

Ran:
`cmake --build --preset default && ctest --test-dir build -j10 -R backend --output-on-failure > test_after.log 2>&1`

Result: build succeeded; CTest exited nonzero because the inventory currently
has 26 non-passing backend-regex-selected tests. Proof log path:
`test_after.log`.
