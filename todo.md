# Execution State

Status: Active
Source Idea Path: ideas/open/59_generic_scalar_instruction_selection_for_x86.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Extract Operand And Legality Selectors
Plan Review Counter: 6 / 10
# Current Packet

## Just Finished

Step 2 extracted the next compare-planning selector seam in
`prepared_local_slot_render.cpp` by factoring prepared immediate compare
branch normalization into
`select_prepared_i32_immediate_branch_plan_if_supported` and by reusing
`render_prepared_i32_compare_and_branch` for the final compare-and-jump text.
The prepared local `i32` and `i16` arithmetic guard families now share one
helper for prepared compare immediate/opcode/label selection instead of
open-coding that planning logic separately in each renderer.

## Suggested Next

Stay in Step 2 and extract the remaining raw immediate-compare normalization
seam so the guard-family renderers stop open-coding separate fallback compare
selection when prepared branch facts are absent, but do not widen yet into
prepared move-bundle call routing, broader arithmetic-family migration, or
Step 3 per-op dispatch.

## Watchouts

- Do not add new whole-function or named-testcase x86 matcher families.
- Consume prepared CFG, value-home, move-bundle, and addressing facts instead
  of re-deriving them from local emitter shape.
- Keep idea 59 focused on scalar instruction selection structure, not on
  reopening upstream ownership from ideas 58, 60, or 61.
- Prefer one coherent instruction-family migration per packet over broad
  emitter rewrites.
- The new helper only normalizes prepared immediate compare-branch facts plus
  the shared compare-and-jump text for the current guard families; raw compare
  fallback discovery inside the `i32` arithmetic guard still remains local and
  is the next obvious Step 2 selector seam.
- The matching `^backend_` before/after logs are not fully green: both
  `test_before.log` and `test_after.log` fail in
  `backend_codegen_route_x86_64_variadic_double_bytes_observe_semantic_bir`,
  `backend_codegen_route_x86_64_variadic_pair_second_observe_semantic_bir`,
  `backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir`,
  and `backend_codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir`.

## Proof

Ran the proof command `cmake --build --preset default && ctest --test-dir
build -j --output-on-failure -R '^backend_'` and captured the output in
`test_after.log`. The build completed successfully after this Step 2 prepared
immediate compare-branch helper extraction, and the `^backend_` subset kept
the same four failing tests already present in `test_before.log`:
`backend_codegen_route_x86_64_variadic_double_bytes_observe_semantic_bir`,
`backend_codegen_route_x86_64_variadic_pair_second_observe_semantic_bir`,
`backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir`,
and `backend_codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir`.
