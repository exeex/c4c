# Execution State

Status: Active
Source Idea Path: ideas/open/62_stack_addressing_and_dynamic_local_access_for_x86_backend.md
Source Plan Path: plan.md
Current Step ID: 2.1
Current Step Title: Repair The Selected Addressing Seam
Plan Review Counter: 2 / 10
# Current Packet

## Just Finished

Completed idea-62 Step 2.1 repair for the first surviving `stdarg`
scalar/local-memory scratch-copy seam by extending scalar-slot local memcpy
lowering in two places:

- decimal floating immediates now lower into scalar scratch initializers such
  as `store double 0.0, ptr %t114`
- pointer-to-scalar scratch memcpy now supports prefix-sized raw byte copies
  into larger scalar slots instead of requiring exact-size matches, covering
  the real `stdarg` lanes such as `store i64 0, ptr %t24` + `memcpy %t23 ->
  %t24, size 7` and later float/double scratch-prefix copies

Added focused `backend_lir_to_bir_notes` success repros for the decimal-zero
double scratch store, partial float-to-double scratch memcpy/load, and partial
7-byte raw-byte-to-`i64` scratch memcpy/load lanes.

`c_testsuite_x86_backend_src_00204_c` no longer fails in semantic
`lir_to_bir` / `scalar-local-memory` ownership. It now reaches the later x86
prepared/emitter restriction `error: x86 backend emitter only supports a
minimal i32 return function through the canonical prepared-module handoff`,
which graduates the case out of idea 62 and into idea 60 ownership.

## Suggested Next

Route `c_testsuite_x86_backend_src_00204_c` into idea 60 and start a new
narrow x86 emitter packet for the first prepared/backend restriction behind the
current `minimal i32 return function through the canonical prepared-module
handoff` error. Keep the new scalar-scratch prefix-copy note repros as fixed
sentinels so idea-62 local-memory ownership does not regress while idea 60
takes over.

## Watchouts

- Adjacent variadic backend routes still pass; do not regress
  `backend_codegen_route_x86_64_variadic_pair_second_observe_semantic_bir` or
  `backend_codegen_route_x86_64_variadic_double_bytes_observe_semantic_bir`
  while downstream idea-60 ownership picks up `00204.c`.
- The scalar scratch-copy lane now includes non-exact prefix memcpys into
  larger scalar slots, not just exact-size transfers. Future local-memory work
  should preserve the new `s7` / float-scratch prefix-copy sentinels.
- Do not reopen idea-62 ownership for `00204.c` unless a later change causes it
  to fall back into semantic addressing or scalar/local-memory failure before
  the prepared/emitter handoff again.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R
'^(backend_lir_to_bir_notes|backend_codegen_route_x86_64_(variadic_pair_second|variadic_double_bytes)_observe_semantic_bir|c_testsuite_x86_backend_src_00204_c)$'`
with output in `test_after.log`. `backend_lir_to_bir_notes`,
`backend_codegen_route_x86_64_variadic_pair_second_observe_semantic_bir`, and
`backend_codegen_route_x86_64_variadic_double_bytes_observe_semantic_bir`
passed, including the focused `local_aggregate_raw_i8_gep_byte_slice`,
`local_aggregate_raw_float_leaf_byte_slice`,
`local_aggregate_raw_float_tail_memcpy`,
`local_scalar_double_decimal_zero_store`,
`local_scalar_double_partial_float_memcpy`, and
`local_scalar_i64_partial_i8_memcpy` notes repros.
`c_testsuite_x86_backend_src_00204_c` still failed, but it advanced out of
semantic `lir_to_bir` ownership and now stops later with
`error: x86 backend emitter only supports a minimal i32 return function through
the canonical prepared-module handoff`.
