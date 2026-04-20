# Execution State

Status: Active
Source Idea Path: ideas/open/59_generic_scalar_instruction_selection_for_x86.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Migrate Covered Terminator And Call Families
Plan Review Counter: 7 / 10
# Current Packet

## Just Finished

Step 4 kept the covered single-block direct extern-call family bounded in
`prepared_local_slot_render.cpp`, but moved one real per-call ABI seam out of
the whole-sequence matcher: the direct extern-call route now uses explicit
helpers to select covered call arguments and finalize covered call-result
carriers instead of open-coding that ABI argument/result handling inline in
`render_prepared_minimal_direct_extern_call_sequence_from_context`.

## Suggested Next

Keep Step 4 in `prepared_local_slot_render.cpp`, but audit the remaining
single-block direct extern-call route for whether one more real helper seam
still exists near return-value finalization or data emission; if the rest is
only wrapper-level glue, hand the next packet off to the next non-cosmetic
covered call or terminator family instead of forcing another thin extraction.

## Watchouts

- Do not add new whole-function or named-testcase x86 matcher families.
- Consume prepared CFG, value-home, move-bundle, and addressing facts instead
  of re-deriving them from local emitter shape.
- Keep idea 59 focused on scalar instruction selection structure, not on
  reopening upstream ownership from ideas 58, 60, or 61.
- Prefer one coherent instruction-family migration per packet over broad
  emitter rewrites.
- The new helpers are intentionally bounded to the covered direct extern-call
  lane; do not widen them into variadic, indirect-call, broader call-result,
  or fallback-family rewrites in the same packet stream.
- The remaining direct extern-call body still owns constant folding, final
  return emission, and private-data/global-data collection, so the next
  packet should confirm that any further extraction would still be a real
  per-call or per-terminator seam.
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
Step 4 direct extern-call ABI-helper extraction. The final `^backend_`
subset in `test_after.log` preserved the accepted `test_before.log` failure
set exactly, with no new failures:
`backend_codegen_route_x86_64_variadic_double_bytes_observe_semantic_bir`,
`backend_codegen_route_x86_64_variadic_pair_second_observe_semantic_bir`,
`backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir`,
and `backend_codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir`.
