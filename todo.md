# Execution State

Status: Active
Source Idea Path: ideas/open/59_generic_scalar_instruction_selection_for_x86.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Migrate Covered Terminator And Call Families
Plan Review Counter: 8 / 10
# Current Packet

## Just Finished

Step 4 kept the covered single-block direct extern-call family bounded in
`prepared_local_slot_render.cpp`, and extracted the last real covered
terminator seam still living inside that matcher: final return-value emission
now routes through
`finalize_prepared_direct_extern_return_if_supported` instead of open-coding
the return-register and immediate-return finalization inline at the end of
`render_prepared_minimal_direct_extern_call_sequence_from_context`.

## Suggested Next

Treat the covered single-block direct extern-call family as structurally
exhausted for Step 4 and move the next packet to the next non-cosmetic
covered call family in `prepared_local_slot_render.cpp`, starting with a
bounded audit of whether `render_prepared_block_same_module_helper_call_inst_if_supported`
still owns one real per-call dispatch seam that should become explicit helper
selection instead of inline family-specific glue.

## Watchouts

- Do not add new whole-function or named-testcase x86 matcher families.
- Consume prepared CFG, value-home, move-bundle, and addressing facts instead
  of re-deriving them from local emitter shape.
- Keep idea 59 focused on scalar instruction selection structure, not on
  reopening upstream ownership from ideas 58, 60, or 61.
- Prefer one coherent instruction-family migration per packet over broad
  emitter rewrites.
- The direct extern-call family now has explicit helpers for covered argument
  selection, result-carrier finalization, and return-value finalization; the
  remaining constant folding and data emission look like route glue, not the
  next Step 4 selector target.
- Do not widen the direct extern-call helpers into variadic, indirect-call,
  broader call-result, or fallback-family rewrites after marking this family
  exhausted.
- The next packet should move to another covered call or terminator family,
  not force another wrapper-level extraction out of the already-bounded
  direct extern-call route.
- Step 4 should keep moving the active route toward prepared per-call or
  per-terminator selection, not back into Step 3 scalar family cleanup or
  into broad single-block fallback rewrites.
- The matching `^backend_` before/after logs are not fully green: both
  `test_before.log` and `test_after.log` fail in
  `backend_codegen_route_x86_64_variadic_double_bytes_observe_semantic_bir`,
  `backend_codegen_route_x86_64_variadic_pair_second_observe_semantic_bir`,
  `backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir`,
  and `backend_codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir`.

## Proof

Ran the proof command `cmake --build --preset default && ctest --test-dir
build -j --output-on-failure -R '^backend_' 2>&1 | tee
/workspaces/c4c/test_after.log`. The build completed successfully after this
Step 4 direct extern-call return-finalization helper extraction. The final
`^backend_` subset in `test_after.log` preserved the accepted
`test_before.log` failure set exactly, with no new failures:
`backend_codegen_route_x86_64_variadic_double_bytes_observe_semantic_bir`,
`backend_codegen_route_x86_64_variadic_pair_second_observe_semantic_bir`,
`backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir`,
and `backend_codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir`.
