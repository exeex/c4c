# Execution State

Status: Active
Source Idea Path: ideas/open/62_stack_addressing_and_dynamic_local_access_for_x86_backend.md
Source Plan Path: plan.md
Current Step ID: 2.1
Current Step Title: Repair The Selected Addressing Seam
Plan Review Counter: 1 / 10
# Current Packet

## Just Finished

Completed idea-62 Step 2.1 repair for the next raw frontend-expanded AMD64
`va_arg` local scratch lane by teaching repeated-extent discovery to treat
consecutive same-typed struct fields as a reusable local tail view. This makes
destination-side raw byte-slice memcpy lowering cover the surviving `hfa14`
register-lane tail copy (`memcpy %t174 -> %t175`, offset `+8`, size `8`) in
addition to the earlier `s9` and `hfa13` byte-slice GEP lanes. Added a focused
`backend_lir_to_bir_notes` success repro for the raw float-tail memcpy case.
`c_testsuite_x86_backend_src_00204_c` now advances past the old `myprintf`
scalar/local-memory blocker and fails later in function `stdarg` under the same
semantic family, so this packet is complete and ready for supervisor review.

## Suggested Next

Start a new narrow packet for the first surviving `stdarg`
scalar/local-memory blocker in `c_testsuite_x86_backend_src_00204_c`, keeping
the `s9`, `hfa13`, and new `hfa14` raw byte-slice/memcpy lanes as fixed
sentinels while isolating the earliest local scratch or aggregate access shape
that still fails after `myprintf` now lowers.

## Watchouts

- Adjacent variadic backend routes still pass; do not regress
  `backend_codegen_route_x86_64_variadic_pair_second_observe_semantic_bir` or
  `backend_codegen_route_x86_64_variadic_double_bytes_observe_semantic_bir`
  while chasing the next scalar/local-memory blocker.
- The destination-side local-aggregate raw byte-slice lane now includes a
  multi-leaf float-tail memcpy path, not just single-leaf byte-slice GEPs. The
  next packet should not reopen that neighborhood unless a new repro says
  otherwise.
- Do not claim idea-62 completion yet: `c_testsuite_x86_backend_src_00204_c`
  still stops before prepared-x86 handoff, but the owned blocker has moved from
  `myprintf` to `stdarg`.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R
'^(backend_lir_to_bir_notes|backend_codegen_route_x86_64_(variadic_pair_second|variadic_double_bytes)_observe_semantic_bir|c_testsuite_x86_backend_src_00204_c)$'`
with output in `test_after.log`. `backend_lir_to_bir_notes`,
`backend_codegen_route_x86_64_variadic_pair_second_observe_semantic_bir`, and
`backend_codegen_route_x86_64_variadic_double_bytes_observe_semantic_bir`
passed, including the focused `local_aggregate_raw_i8_gep_byte_slice`,
`local_aggregate_raw_float_leaf_byte_slice`, and
`local_aggregate_raw_float_tail_memcpy` notes repros;
`c_testsuite_x86_backend_src_00204_c` still failed, but the latest function
failure advanced from `myprintf` to `stdarg` under `scalar/local-memory
semantic family`.
