# Execution State

Status: Active
Source Idea Path: ideas/open/59_generic_scalar_instruction_selection_for_x86.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Migrate Covered Terminator And Call Families
Plan Review Counter: 6 / 10
# Current Packet

## Just Finished

Step 4 extracted the covered direct extern-call lane in the
single-block return dispatch path in `prepared_local_slot_render.cpp` into
the named helper
`render_prepared_single_block_return_direct_extern_call_if_supported`, so
that prepared per-call handoff now routes through one explicit helper
instead of calling the direct extern-call renderer inline from
`render_prepared_single_block_return_dispatch_if_supported`.

## Suggested Next

Keep Step 4 bounded to a short audit packet in
`prepared_local_slot_render.cpp` that checks whether any real covered
per-call or per-terminator seam still remains near the single-block return
dispatch path; if not, explicitly hand the next Step 4 packet off to the
next non-cosmetic call or terminator family instead of forcing another thin
wrapper extraction.

## Watchouts

- Do not add new whole-function or named-testcase x86 matcher families.
- Consume prepared CFG, value-home, move-bundle, and addressing facts instead
  of re-deriving them from local emitter shape.
- Keep idea 59 focused on scalar instruction selection structure, not on
  reopening upstream ownership from ideas 58, 60, or 61.
- Prefer one coherent instruction-family migration per packet over broad
  emitter rewrites.
- The new helper is intentionally bounded to the covered direct extern-call
  lane in `render_prepared_single_block_return_dispatch_if_supported`; do
  not widen it into variadic, indirect-call, broader call-result, or
  fallback-family rewrites in the same packet.
- The nearby single-block return fallbacks are already mostly named helpers,
  so the next packet should confirm a real Step 4 seam exists before adding
  another wrapper-level extraction.
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
Step 4 direct extern-call helper extraction. The final `^backend_` subset in
`test_after.log` preserved the accepted `test_before.log` failure set
exactly, with no new failures:
`backend_codegen_route_x86_64_variadic_double_bytes_observe_semantic_bir`,
`backend_codegen_route_x86_64_variadic_pair_second_observe_semantic_bir`,
`backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir`,
and `backend_codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir`.
