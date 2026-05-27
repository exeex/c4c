# Current Packet

Status: Active
Source Idea Path: ideas/open/55_aarch64_scalar_cast_alu_publication_prepared_authority_repair.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Establish scalar cast ALU publication baseline

## Just Finished

Step 1 classified the remaining `00204` scalar publication mismatch. The bad
sequence is in `myprintf` after `vaarg.join.39`:

`mov w9, w13; sxtw x9, w9`

It materializes `%t45.byte_offset.i64 = bir.sext i32 %t35 to i64` while the
current join context has also published `%t49` in `x13`. The scalar cast source
selection therefore reads stale `w13` as the original `%t35` gp_offset, even
though `x13` now carries the selected pointer.

Exact emitter/helper path:

- `src/backend/mir/aarch64/codegen/cast_ops.cpp`:
  `lower_scalar_cast_publication_to_prepared_stack` publishes the stack-home
  cast result `%t45.byte_offset.i64`.
- That helper calls
  `src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp`:
  `emit_value_publication_to_register` for the cast result.
- The `CastOpcode::SExt` branch recursively publishes `%t35` into scratch
  `x9`, then emits `sxtw x9, w9`.

Prepared evidence:

- Prepared BIR moves the cast/add into `vaarg.join.39`.
- `home %t35` is `register reg=x13`, while `home %t49` is also `register
  reg=x13` in the active dirty context.
- The prepared join-transfer authority for `%t49` says
  `carrier=select_materialization`, with edge transfers `%t45 -> %t49` and
  `%t47 -> %t49`.
- The prepared memory accesses for the byte copy correctly identify
  `base=pointer_value pointer=%t49 offset=0..8`.

Prepared authority the next packet should consume:

- For the `%t35` source of `%t45.byte_offset.i64`, prefer a prepared
  scalar-cast consumer source that proves the original `%t35` register/home is
  still current before accepting `find_emitted_scalar_register`.
- If the register has been superseded by block-entry or edge-publication
  authority for `%t49`, reload `%t35` from its prepared value home/source or use
  a prepared scalar-publication/source-producer fact instead of stale
  `scalar_state`.
- For `%t49` byte-copy address consumers, continue to trust the prepared
  pointer-value access facts and join-transfer source-producer authority; do
  not reinterpret the selected pointer as the integer offset.

## Suggested Next

Implement a small shared-query repair used by scalar cast publication first:
teach the cast publication path to prefer an authoritative prepared consumer
source for `%t35` over `find_emitted_scalar_register` when block-entry or
edge-publication state has republished the same physical register for `%t49`.
Owned code should include `cast_ops.cpp` plus the smallest shared lookup/helper
surface needed to ask whether the emitted scalar register is still current;
`dispatch_value_materialization.cpp` is only involved if the shared query must
be consumed from `emit_value_publication_to_register`.

## Watchouts

Do not accept or commit the dirty `memory.cpp` and `dispatch_edge_copies.cpp`
changes as part of this lifecycle reset. They remain incomplete context that
removed the original `00204` segfault and bad cursor reload, but focused proof
still fails `00204`.

This is not ALU-only: the bad instruction is a scalar cast publication for
`%t45.byte_offset.i64`, not the pointer `add` itself. It is not dispatch
value-materialization-only either: the dispatch helper is upstream plumbing,
but the stale source choice is accepted by scalar cast publication before the
byte-copy address consumers run.

Do not fix this by special-casing `%t35`, `%t45`, `%t49`, `vaarg.join.39`, or
`00204`. The generalized rule is that scalar cast publication must not trust an
emitted register mapping when prepared block-entry or edge publication has made
that physical register authoritative for a different value.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_codegen_route_aarch64_pointer_select_aggregate_byte_copy|backend_codegen_route_aarch64_variadic_aggregate_overflow_byte_copy|backend_codegen_route_aarch64_alu_unpublished_load_local_(after_call|call_boundary)|c_testsuite_aarch64_backend_src_00164_c|c_testsuite_aarch64_backend_src_00176_c|c_testsuite_aarch64_backend_src_00181_c|c_testsuite_aarch64_backend_src_00204_c)$'`

Result: `7/8` passed. The only failing test is
`c_testsuite_aarch64_backend_src_00204_c`, still failing as
`[RUNTIME_MISMATCH]`.

Proof log path: `test_after.log`.

Diagnostic dumps inspected:

- `/tmp/00204.myprintf.bir.txt`
- `/tmp/00204.myprintf.prepared-bir.txt`
- `/tmp/00204.s`
