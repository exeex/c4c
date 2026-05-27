# Current Packet

Status: Active
Source Idea Path: ideas/open/56_aarch64_edge_terminator_consumer_preservation_repair.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Establish edge preservation baseline

## Just Finished

Lifecycle split from scalar cast/ALU publication to edge/terminator consumer
preservation. The committed Step 3 classification showed that the missing fact
is predecessor-side preservation or placement for `%t35 ->
%t45.byte_offset.i64` before out-of-SSA edge publication makes `x13`
authoritative for `%t49`.

## Suggested Next

Run Step 1: establish the edge preservation baseline. Use the eight-test proof
subset, inspect prepared BIR around `%t35`, `%t45.byte_offset.i64`, `%t49`, and
`vaarg.join.39`, and identify the exact prepared functions/data structures that
own the late consumer move and the predecessor edge clobber.

## Watchouts

Do not edit or revert the existing dirty `memory.cpp` and
`dispatch_edge_copies.cpp` context during lifecycle-only work. They remain
incomplete implementation context until a supervisor accepts, splits, or
reroutes them.

Do not reload `%lv.ap.24` in the join block. Do not special-case `00204`,
`myprintf`, `%t35`, `%t45`, `%t49`, `vaarg.join.39`, or `x13`.

## Proof

Required focused proof command:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_codegen_route_aarch64_pointer_select_aggregate_byte_copy|backend_codegen_route_aarch64_variadic_aggregate_overflow_byte_copy|backend_codegen_route_aarch64_alu_unpublished_load_local_(after_call|call_boundary)|c_testsuite_aarch64_backend_src_00164_c|c_testsuite_aarch64_backend_src_00176_c|c_testsuite_aarch64_backend_src_00181_c|c_testsuite_aarch64_backend_src_00204_c)$'`

Last known result before this lifecycle split: `7/8` passed. The only failing
test was `c_testsuite_aarch64_backend_src_00204_c` with
`[RUNTIME_MISMATCH]`. This lifecycle-only reset did not rerun validation.
