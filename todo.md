# Current Packet

Status: Active
Source Idea Path: ideas/open/55_aarch64_scalar_cast_alu_publication_prepared_authority_repair.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Establish scalar cast ALU publication baseline

## Just Finished

Lifecycle reset from the dispatch value-materialization route after
`review/dispatch-step3-route-review.md` judged the next direct
`cast_ops.cpp` / `alu.cpp` packet outside idea 49. The new source idea owns the
actual remaining scalar cast/ALU publication surface for the `00204`
`[RUNTIME_MISMATCH]`.

## Suggested Next

Execute Step 1: classify the exact cast or ALU publication emitter for the
remaining `mov w9, w13; sxtw x9, w9` sequence around
`%t45.byte_offset.i64` / `%t45`, record the prepared source/home authority it
should consume, and decide whether the next implementation packet is cast-only,
ALU-only, or a small shared-query repair.

## Watchouts

Do not accept or commit the dirty `memory.cpp` and `dispatch_edge_copies.cpp`
changes as part of this lifecycle reset. They remain incomplete context that
removed the original `00204` segfault and bad cursor reload, but focused proof
still fails `00204`.

Keep the dispatch, calls, ALU, memory, and comparison ideas parked open. Do not
widen this route back into dispatch value materialization unless Step 1 proves
that cast/ALU consumers need a specific shared query from that surface.

## Proof

Required focused subset for the next executor packet:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_codegen_route_aarch64_pointer_select_aggregate_byte_copy|backend_codegen_route_aarch64_variadic_aggregate_overflow_byte_copy|backend_codegen_route_aarch64_alu_unpublished_load_local_(after_call|call_boundary)|c_testsuite_aarch64_backend_src_00164_c|c_testsuite_aarch64_backend_src_00176_c|c_testsuite_aarch64_backend_src_00181_c|c_testsuite_aarch64_backend_src_00204_c)$'`

Current known result before this reset: `7/8` passed, with only
`c_testsuite_aarch64_backend_src_00204_c` failing as `[RUNTIME_MISMATCH]`.
Proof log path remains `test_after.log`; lifecycle reset did not rerun
validation.
