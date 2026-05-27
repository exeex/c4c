# Current Packet

Status: Active
Source Idea Path: ideas/open/52_aarch64_calls_prepared_authority_repair.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Repair by-value aggregate argument lane lowering

## Just Finished

Step 3 implemented the prepared call planning/source-selection repair for
small single-GPR direct variadic by-value aggregate payloads in
`src/backend/prealloc/call_plans.cpp`.

`prepared_byval_lane_extent_bytes` now preserves the existing
`call_arg_byval_aggregate_register_lanes` route and also recognizes a direct
extern variadic, by-value, integer-class aggregate payload when the prepared
before-call move is still classified as scalar
`call_arg_register_to_register`, the ABI destination is the matching
single-GPR call argument binding, and the source home is available.

Generated prepared output for `00204` now records `ByvalRegisterLane` source
selection with 8-byte lane extents for the small one-GPR aggregate lanes, and
the AArch64 output consumes those prepared stack bytes for the relevant call
arguments. No testcase names, temporary names, format strings, string literal
contents, or expectation changes were used.

## Suggested Next

Delegate the next packet to classify the remaining `00204` stdarg string
runtime mismatch after the small one-GPR prepared lane facts are present and
consumed. The next owner should inspect the remaining variadic string pointer
or va_list overflow/register-save publication route rather than reworking
prepared call argument source selection first.

## Watchouts

Existing multi-lane `ByvalRegisterLane` behavior remains on the explicit
`call_arg_byval_aggregate_register_lanes` path. The new scalar-move branch is
limited to direct extern variadic single-GPR integer by-value aggregate lanes.

The focused proof still fails only `c_testsuite_aarch64_backend_src_00204_c`
with `[RUNTIME_MISMATCH]`. The observed actual output still corrupts the
`stdarg:` string section even though the new small-lane prepared facts are
present, so acceptance remains blocked outside this completed planning slice.

Treat existing dirty `memory.cpp`, `dispatch_edge_copies.cpp`, and transient
`review/*` files as external context.

## Proof

Proof command:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_codegen_route_aarch64_pointer_select_aggregate_byte_copy|backend_codegen_route_aarch64_variadic_aggregate_overflow_byte_copy|backend_codegen_route_aarch64_alu_unpublished_load_local_(after_call|call_boundary)|c_testsuite_aarch64_backend_src_00164_c|c_testsuite_aarch64_backend_src_00176_c|c_testsuite_aarch64_backend_src_00181_c|c_testsuite_aarch64_backend_src_00204_c)$'`

Result: build passed, focused subset `7/8` passed, and only
`c_testsuite_aarch64_backend_src_00204_c` failed with `[RUNTIME_MISMATCH]`.
Proof log path: `test_after.log`.
