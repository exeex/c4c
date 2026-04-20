# Execution State

Status: Active
Source Idea Path: ideas/open/59_generic_scalar_instruction_selection_for_x86.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Migrate Covered Scalar Instruction Families
Plan Review Counter: 3 / 10
# Current Packet

## Just Finished

Step 3 extracted `render_prepared_ptr_store_to_memory_if_supported` in
`x86_codegen.hpp` and rewired the prepared local-slot guard-chain store path
plus the nearby minimal local-slot return store path in
`prepared_local_slot_render.cpp` to consume shared scalar store-family
renderers instead of spelling final pointer-store writes inline. The minimal
local-slot return route now also reuses the existing shared
`render_prepared_i32_store_to_memory_if_supported` helper for its immediate
`i32` store lane.

## Suggested Next

Keep Step 3 bounded by moving to another single covered scalar family rather
than stretching the new store helper contracts. The remaining nearby store
lanes are mostly `i8` or same-module-global specifics, so only widen this
area if another real shared scalar selector seam appears instead of forcing a
type- or testcase-shaped helper.

## Watchouts

- Do not add new whole-function or named-testcase x86 matcher families.
- Consume prepared CFG, value-home, move-bundle, and addressing facts instead
  of re-deriving them from local emitter shape.
- Keep idea 59 focused on scalar instruction selection structure, not on
  reopening upstream ownership from ideas 58, 60, or 61.
- Prefer one coherent instruction-family migration per packet over broad
  emitter rewrites.
- The new shared ptr-store helper only owns final address-to-memory spelling;
  keep carrier-state updates, value-home lookup, and prepared-memory
  selection at the call sites until a broader selector contract is actually
  justified.
- The bounded same-module helper route still has an `i32`-only store lane, so
  do not force the next packet around pointer-store support where the route
  does not already consume prepared value-home facts.
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
Step 3 bounded scalar store-family slice. A focused repair pass also reran
`ctest --test-dir build --output-on-failure -R '^backend_x86_handoff_boundary$'`
to clear an intermediate handoff-boundary regression introduced while
extracting the ptr-store helper. The final `^backend_` subset in
`test_after.log` matched the accepted `test_before.log` failure set exactly:
`backend_codegen_route_x86_64_variadic_double_bytes_observe_semantic_bir`,
`backend_codegen_route_x86_64_variadic_pair_second_observe_semantic_bir`,
`backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir`,
and `backend_codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir`.
