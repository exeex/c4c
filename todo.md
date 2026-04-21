# Execution State

Status: Active
Source Idea Path: ideas/open/62_stack_addressing_and_dynamic_local_access_for_x86_backend.md
Source Plan Path: plan.md
Current Step ID: 2.1
Current Step Title: Repair The Selected Addressing Seam
Plan Review Counter: 0 / 10
# Current Packet

## Just Finished

Attempted idea-62 Step 2.1 repair for the raw frontend-expanded AMD64
`va_arg` lane by lowering raw `getelementptr i8, ptr %base, i64 %offset`
into pointer-valued BIR adds and by admitting immediate local `memcpy` from a
raw pointer SSA into local scalar/aggregate storage. This shrank the smallest
raw-expanded repros: `va_arg(ap, int)` now advances past semantic `gep` into a
later prepared compare-join blocker, and minimal aggregate `va_arg` repros no
longer fail inside the variadic callee. Repaired destination-side raw
byte-slice GEP support for local aggregate scratch allocas when the byte offset
lands on a tracked scalar leaf, covering the original `s9` split-copy lane
(`%t49` / `%t51 = getelementptr i8, ptr %t49, i64 8`) and the next `hfa13`
float-leaf lane (`%t140` / `%t142 = getelementptr i8, ptr %t140, i64 8`).
Added focused `backend_lir_to_bir_notes` success repros for both an `i8` leaf
byte-slice and a non-`i8` float-leaf byte-slice. `c_testsuite_x86_backend_src_00204_c`
now advances past `gep local-memory semantic family` and stops later in
`myprintf` under `scalar/local-memory semantic family`, so this packet is
complete but the overall slice remains uncommitted.

## Suggested Next

Start a new narrow packet for the next `myprintf` blocker in
`scalar/local-memory semantic family`, using the now-green byte-slice GEP lanes
as fixed sentinels and isolating the first surviving scalar/local-memory shape
after the `hfa13` split-copy handoff.

## Watchouts

- Adjacent variadic backend routes still pass; do not regress
  `backend_codegen_route_x86_64_variadic_pair_second_observe_semantic_bir` or
  `backend_codegen_route_x86_64_variadic_double_bytes_observe_semantic_bir`
  while chasing the next scalar/local-memory blocker.
- The destination-side local-aggregate raw byte-slice GEP lane is no longer
  the live blocker. The next packet should not reopen it unless a new repro
  says otherwise.
- Do not claim idea-62 completion yet: `c_testsuite_x86_backend_src_00204_c`
  now stops later in `myprintf` under `scalar/local-memory semantic family`.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R
'^(backend_lir_to_bir_notes|backend_codegen_route_x86_64_(variadic_pair_second|variadic_double_bytes)_observe_semantic_bir|c_testsuite_x86_backend_src_00204_c)$'`
with output in `test_after.log`. `backend_lir_to_bir_notes`,
`backend_codegen_route_x86_64_variadic_pair_second_observe_semantic_bir`, and
`backend_codegen_route_x86_64_variadic_double_bytes_observe_semantic_bir`
passed, including the focused `local_aggregate_raw_i8_gep_byte_slice` and
`local_aggregate_raw_float_leaf_byte_slice` notes repros;
`c_testsuite_x86_backend_src_00204_c` still failed with `latest function
failure: semantic lir_to_bir function 'myprintf' failed in scalar/local-memory
semantic family`.
