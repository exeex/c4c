# Execution State

Status: Active
Source Idea Path: ideas/open/59_generic_scalar_instruction_selection_for_x86.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Migrate Covered Scalar Instruction Families
Plan Review Counter: 9 / 10
# Current Packet

## Just Finished

Step 3 extracted the remaining scalar load/store lane in
`prepared_local_slot_render.cpp` into dedicated per-op helpers,
`render_prepared_scalar_load_inst_if_supported` and
`render_prepared_scalar_store_inst_if_supported`, so the block-local
`rendered_load_or_store` lambda now dispatches through reusable selector-style
helpers instead of owning the covered local/global load/store families inline.

## Suggested Next

Reassess Step 3 for any remaining real scalar instruction-family seams in the
prepared local-slot renderer; if the scalar path is now structurally
exhausted, hand the next packet off to Step 4 call or terminator migration
instead of forcing another cosmetic Step 3 refactor.

## Watchouts

- Do not add new whole-function or named-testcase x86 matcher families.
- Consume prepared CFG, value-home, move-bundle, and addressing facts instead
  of re-deriving them from local emitter shape.
- Keep idea 59 focused on scalar instruction selection structure, not on
  reopening upstream ownership from ideas 58, 60, or 61.
- Prefer one coherent instruction-family migration per packet over broad
  emitter rewrites.
- The new load/store helpers are intentionally limited to the already-covered
  scalar local/global load/store families plus their register-state updates;
  do not widen them into call, terminator, or whole-block dispatch ownership.
- The pointer-store helper path now renders stack addresses from prepared
  value-home frame offsets inline inside the helper; keep any follow-up work
  anchored on prepared ownership rather than ad hoc local-slot inference.
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
Step 3 bounded load/store helper slice. The final `^backend_` subset in
`test_after.log` matched the accepted `test_before.log` failure set exactly:
`backend_codegen_route_x86_64_variadic_double_bytes_observe_semantic_bir`,
`backend_codegen_route_x86_64_variadic_pair_second_observe_semantic_bir`,
`backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir`,
and `backend_codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir`.
