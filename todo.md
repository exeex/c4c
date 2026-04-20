# Execution State

Status: Active
Source Idea Path: ideas/open/59_generic_scalar_instruction_selection_for_x86.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Migrate Covered Terminator And Call Families
Plan Review Counter: 5 / 10
# Current Packet

## Just Finished

Step 4 extracted the covered same-module helper-call lane in
`prepared_local_slot_render.cpp` out of the inline `render_block` path and
into the named helper
`render_prepared_block_same_module_helper_call_inst_if_supported`, so the
bounded prepared helper-call dispatch now routes through one dedicated
per-call helper instead of invoking the bounded same-module helper-call
renderer directly inside the block loop.

## Suggested Next

Keep Step 4 bounded to the next covered call-family seam in
`prepared_local_slot_render.cpp` by extracting the remaining inline direct
extern-call lane in the single-block return dispatch path onto one named
prepared per-call helper, without widening into variadic, indirect-call, or
branch-plan rewrites.

## Watchouts

- Do not add new whole-function or named-testcase x86 matcher families.
- Consume prepared CFG, value-home, move-bundle, and addressing facts instead
  of re-deriving them from local emitter shape.
- Keep idea 59 focused on scalar instruction selection structure, not on
  reopening upstream ownership from ideas 58, 60, or 61.
- Prefer one coherent instruction-family migration per packet over broad
  emitter rewrites.
- The new helper is intentionally bounded to the covered same-module
  helper-call lane in `render_block`; do not widen it into indirect-call,
  variadic, external-call, or broader call-result rewrites in the same
  packet.
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
Step 4 same-module helper-call lane extraction. The final `^backend_` subset
in `test_after.log` preserved the accepted `test_before.log` failure set
exactly, with no new failures:
`backend_codegen_route_x86_64_variadic_double_bytes_observe_semantic_bir`,
`backend_codegen_route_x86_64_variadic_pair_second_observe_semantic_bir`,
`backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir`,
and `backend_codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir`.
