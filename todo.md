# Execution State

Status: Active
Source Idea Path: ideas/open/59_generic_scalar_instruction_selection_for_x86.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Migrate Covered Scalar Instruction Families
Plan Review Counter: 5 / 10
# Current Packet

## Just Finished

Step 3 migrated the prepared local-slot named-`i8` store lane in
`prepared_local_slot_render.cpp` onto a shared
`render_prepared_i8_store_to_memory_if_supported` helper and proved the route
with a focused byte-copy guard handoff test that now emits
`mov BYTE PTR [...], al` from the tracked prepared `current_i8_name` carrier
instead of leaving the store family as an immediate-only inline special case.

## Suggested Next

Keep Step 3 bounded by moving to one adjacent scalar family that still has
route-local emission, likely the remaining same-module or local byte-store
seam if it can reuse the new `i8` helper contract, or otherwise another
nearby per-op family such as select/load plumbing that removes inline
lowering without widening into matcher growth.

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
- The new `i8` store helper is intentionally narrow: it covers immediate byte
  stores and named byte stores only when the prepared route already tracks the
  value in `al` via `current_i8_name`; do not widen it into ad hoc byte-value
  materialization.
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
Step 3 bounded local byte-store helper slice. The final `^backend_`
subset in `test_after.log` matched the accepted `test_before.log` failure set
exactly:
`backend_codegen_route_x86_64_variadic_double_bytes_observe_semantic_bir`,
`backend_codegen_route_x86_64_variadic_pair_second_observe_semantic_bir`,
`backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir`,
and `backend_codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir`.
