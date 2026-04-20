# Execution State

Status: Active
Source Idea Path: ideas/open/59_generic_scalar_instruction_selection_for_x86.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Migrate Covered Scalar Instruction Families
Plan Review Counter: 6 / 10
# Current Packet

## Just Finished

Step 3 migrated the prepared same-module `i8` global load/store lane in
`prepared_local_slot_render.cpp` onto the existing shared scalar
load/store helpers, so `LoadGlobalInst` now tracks named byte results through
`current_i8_name` and `StoreGlobalInst` can emit scalar byte globals through
`render_prepared_i8_store_to_memory_if_supported` instead of leaving that
family behind `i32`-only gating.

## Suggested Next

Keep Step 3 bounded by moving to one adjacent scalar family that still has
route-local emission, likely the remaining local/global byte scalar seam that
still depends on ad hoc carrier tracking or another nearby per-op family such
as select plumbing that removes inline lowering without widening into matcher
growth.

## Watchouts

- Do not add new whole-function or named-testcase x86 matcher families.
- Consume prepared CFG, value-home, move-bundle, and addressing facts instead
  of re-deriving them from local emitter shape.
- Keep idea 59 focused on scalar instruction selection structure, not on
  reopening upstream ownership from ideas 58, 60, or 61.
- Prefer one coherent instruction-family migration per packet over broad
  emitter rewrites.
- The same-module `i8` global store slice is intentionally narrow: it only
  accepts scalar byte globals at `byte_offset == 0` and named byte stores when
  the prepared route already tracks the value in `al` via `current_i8_name`;
  do not widen it into aggregate-byte element stores or ad hoc byte-value
  materialization without a clearer selector contract.
- Preserving the older same-module `i32` store validator matters for the
  existing fixed-offset handoff boundary coverage; do not collapse the `i32`
  and `i8` global-store validators into one stricter byte-offset rule.
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
Step 3 bounded same-module byte-global slice. The final `^backend_`
subset in `test_after.log` matched the accepted `test_before.log` failure set
exactly:
`backend_codegen_route_x86_64_variadic_double_bytes_observe_semantic_bir`,
`backend_codegen_route_x86_64_variadic_pair_second_observe_semantic_bir`,
`backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir`,
and `backend_codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir`.
