# Execution State

Status: Active
Source Idea Path: ideas/open/59_generic_scalar_instruction_selection_for_x86.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Migrate Covered Scalar Instruction Families
Plan Review Counter: 2 / 10
# Current Packet

## Just Finished

Step 3 extracted `render_prepared_scalar_load_from_memory_if_supported` in
`x86_codegen.hpp` and rewired the prepared guard-chain local/global load
paths plus the nearby minimal local-slot return load path in
`prepared_local_slot_render.cpp` to consume that shared load-family
renderer instead of spelling the final register load instructions inline.

## Suggested Next

Keep Step 3 bounded by moving to another single covered scalar family rather
than stretching the new load helper contract: the earlier same-module-helper
`LoadGlobalInst` idea was a false seam, so only extend this area if another
real consumer appears nearby.

## Watchouts

- Do not add new whole-function or named-testcase x86 matcher families.
- Consume prepared CFG, value-home, move-bundle, and addressing facts instead
  of re-deriving them from local emitter shape.
- Keep idea 59 focused on scalar instruction selection structure, not on
  reopening upstream ownership from ideas 58, 60, or 61.
- Prefer one coherent instruction-family migration per packet over broad
  emitter rewrites.
- The new shared load helper only owns final memory-to-register spelling;
  keep carrier-state updates, same-module global selection, and local
  prepared-memory lookup at the call sites until a broader selector contract
  is actually justified.
- The bounded same-module helper route does not currently have a
  `LoadGlobalInst` lane, so do not force the next packet around that stale
  assumption.
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
Step 3 bounded scalar load-family slice, and the `^backend_` subset kept the
same four failing tests already present in `test_before.log`:
`backend_codegen_route_x86_64_variadic_double_bytes_observe_semantic_bir`,
`backend_codegen_route_x86_64_variadic_pair_second_observe_semantic_bir`,
`backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir`,
and `backend_codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir`.
