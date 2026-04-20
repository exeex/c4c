# Execution State

Status: Active
Source Idea Path: ideas/open/59_generic_scalar_instruction_selection_for_x86.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Extract Operand And Legality Selectors
Plan Review Counter: 9 / 10
# Current Packet

## Just Finished

Step 2 extracted the remaining shared `i32` compare-planning seam in
`prepared_local_slot_render.cpp` by factoring prepared-vs-raw branch-plan
selection plus compared-value discovery into
`select_prepared_or_raw_i32_compared_immediate_branch_plan_if_supported`.
The local `i32` arithmetic guard now consumes one helper-returned compared
value and branch plan instead of open-coding separate prepared scan and raw
fallback flows.

## Suggested Next

Stay in Step 2 and audit whether one last selector-seam remains around local
`i32` guard candidate-expression collection, or whether compare-planning
helper extraction is structurally exhausted and the next packet should move to
another Step 2 selector surface, but do not widen yet into prepared
move-bundle call routing, broader arithmetic family migration, or Step 3
per-op dispatch.

## Watchouts

- Do not add new whole-function or named-testcase x86 matcher families.
- Consume prepared CFG, value-home, move-bundle, and addressing facts instead
  of re-deriving them from local emitter shape.
- Keep idea 59 focused on scalar instruction selection structure, not on
  reopening upstream ownership from ideas 58, 60, or 61.
- Prefer one coherent instruction-family migration per packet over broad
  emitter rewrites.
- The `i32` and `i16` local arithmetic guards now share the prepared-or-raw
  compare-plan surface, but candidate-expression discovery for the `i32` guard
  is still assembled inline from `named_i32_exprs` and may be the last nearby
  Step 2 seam worth auditing before moving on.
- The matching `^backend_` before/after logs are not fully green: both
  `test_before.log` and `test_after.log` fail in
  `backend_codegen_route_x86_64_variadic_double_bytes_observe_semantic_bir`,
  `backend_codegen_route_x86_64_variadic_pair_second_observe_semantic_bir`,
  `backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir`,
  and `backend_codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir`.

## Proof

Ran the proof command `cmake --build --preset default && ctest --test-dir
build -j --output-on-failure -R '^backend_'` and captured the output in
`test_after.log`. The build completed successfully after this Step 2 `i32`
compared-value/branch-plan helper extraction, and the `^backend_` subset kept
the same four failing tests already present in `test_before.log`:
`backend_codegen_route_x86_64_variadic_double_bytes_observe_semantic_bir`,
`backend_codegen_route_x86_64_variadic_pair_second_observe_semantic_bir`,
`backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir`,
and `backend_codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir`.
