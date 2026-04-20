# Execution State

Status: Active
Source Idea Path: ideas/open/59_generic_scalar_instruction_selection_for_x86.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Extract Operand And Legality Selectors
Plan Review Counter: 7 / 10
# Current Packet

## Just Finished

Step 2 extracted the raw immediate-compare fallback seam in
`prepared_local_slot_render.cpp` by factoring the `i32` arithmetic guard's
inline final-compare discovery into
`select_raw_i32_immediate_branch_plan_if_supported`. The current local `i32`
guard path now normalizes compared-value, immediate, opcode, and branch-label
selection through helper-returned plan data instead of open-coding that raw
fallback beside the prepared-branch path.

## Suggested Next

Stay in Step 2 and extract the adjacent remaining compare-normalization seam in
the local arithmetic guard helpers, most likely by lifting the `i16` guard's
inline compare fallback onto the same selector-style planning surface, but do
not widen yet into prepared move-bundle call routing, broader arithmetic
family migration, or Step 3 per-op dispatch.

## Watchouts

- Do not add new whole-function or named-testcase x86 matcher families.
- Consume prepared CFG, value-home, move-bundle, and addressing facts instead
  of re-deriving them from local emitter shape.
- Keep idea 59 focused on scalar instruction selection structure, not on
  reopening upstream ownership from ideas 58, 60, or 61.
- Prefer one coherent instruction-family migration per packet over broad
  emitter rewrites.
- The new helper only covers the raw fallback compare discovery for the local
  `i32` arithmetic guard; the `i16` guard still keeps its compare/immediate
  normalization inline and is the next obvious Step 2 selector seam.
- The matching `^backend_` before/after logs are not fully green: both
  `test_before.log` and `test_after.log` fail in
  `backend_codegen_route_x86_64_variadic_double_bytes_observe_semantic_bir`,
  `backend_codegen_route_x86_64_variadic_pair_second_observe_semantic_bir`,
  `backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir`,
  and `backend_codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir`.

## Proof

Ran the proof command `cmake --build --preset default && ctest --test-dir
build -j --output-on-failure -R '^backend_'` and captured the output in
`test_after.log`. The build completed successfully after this Step 2 raw
immediate-compare fallback helper extraction, and the `^backend_` subset kept
the same four failing tests already present in `test_before.log`:
`backend_codegen_route_x86_64_variadic_double_bytes_observe_semantic_bir`,
`backend_codegen_route_x86_64_variadic_pair_second_observe_semantic_bir`,
`backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir`,
and `backend_codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir`.
