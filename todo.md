# Execution State

Status: Active
Source Idea Path: ideas/open/59_generic_scalar_instruction_selection_for_x86.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Migrate Covered Scalar Instruction Families
Plan Review Counter: 4 / 10
# Current Packet

## Just Finished

Step 3 rewired the general prepared local-slot guard-chain same-module
`StoreGlobalInst` `i32` lane in `prepared_local_slot_render.cpp` to reuse the
shared `render_prepared_i32_store_to_memory_if_supported` helper instead of
spelling the final global-memory `mov` variants inline. This keeps the
same-module scalar store route aligned with the nearby bounded same-module
helper path and continues the per-op store-family migration without widening
the helper contract.

## Suggested Next

Keep Step 3 bounded by moving to another single covered scalar family rather
than stretching the store helpers beyond real reuse. The remaining nearby
scalar seams appear to be the local `i8` store lane or another adjacent
family such as select/load plumbing, so only continue in this area if the
next packet still produces a reusable legality surface instead of a
type- or testcase-shaped shortcut.

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
- The same-module global-store routes now share the final `i32` memory-store
  renderer, but the surrounding prepared-memory selection and state tracking
  remain route-local; do not widen that ownership unless another real shared
  selector seam appears.
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
Step 3 bounded same-module scalar store-helper slice. The final `^backend_`
subset in `test_after.log` matched the accepted `test_before.log` failure set
exactly:
`backend_codegen_route_x86_64_variadic_double_bytes_observe_semantic_bir`,
`backend_codegen_route_x86_64_variadic_pair_second_observe_semantic_bir`,
`backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir`,
and `backend_codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir`.
