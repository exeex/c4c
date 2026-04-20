# Execution State

Status: Active
Source Idea Path: ideas/open/59_generic_scalar_instruction_selection_for_x86.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Migrate Covered Terminator And Call Families
Plan Review Counter: 9 / 10
# Current Packet

## Just Finished

Step 4 moved one real per-call seam out of the covered same-module
helper-call lane in `prepared_local_slot_render.cpp`: the helper-call route
now uses `finalize_prepared_same_module_helper_call_state` to perform the
post-call result-carrier and state reset handoff instead of open-coding that
state transition inline inside
`render_prepared_bounded_same_module_helper_call_if_supported`.

## Suggested Next

Keep Step 4 in the same-module helper-call family for one short audit packet:
check whether any real covered per-call seam still remains near call spelling
or the block-loop handoff in
`render_prepared_block_same_module_helper_call_inst_if_supported`; if the
rest is only wrapper glue, mark this helper-call lane structurally exhausted
and hand the next packet to the next non-cosmetic covered call or terminator
family.

## Watchouts

- Do not add new whole-function or named-testcase x86 matcher families.
- Consume prepared CFG, value-home, move-bundle, and addressing facts instead
  of re-deriving them from local emitter shape.
- Keep idea 59 focused on scalar instruction selection structure, not on
  reopening upstream ownership from ideas 58, 60, or 61.
- Prefer one coherent instruction-family migration per packet over broad
  emitter rewrites.
- The same-module helper-call lane now has explicit selector-style argument
  moves and an explicit post-call state/result helper; the next packet should
  confirm there is still a real per-call seam before extracting anything
  else from that route.
- Do not widen the same-module helper-call lane into variadic, indirect-call,
  generic non-helper call families, or fallback-family rewrites in this
  packet stream.
- The direct extern-call family is already structurally exhausted for Step 4;
  stay on the same-module helper-call lane until it is either exhausted or
  yields one more real seam.
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
Step 4 same-module helper-call state-finalization extraction. The final
`^backend_` subset in `test_after.log` preserved the accepted
`test_before.log` failure set exactly, with no new failures:
`backend_codegen_route_x86_64_variadic_double_bytes_observe_semantic_bir`,
`backend_codegen_route_x86_64_variadic_pair_second_observe_semantic_bir`,
`backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir`,
and `backend_codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir`.
