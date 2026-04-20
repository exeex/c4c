# Execution State

Status: Active
Source Idea Path: ideas/open/59_generic_scalar_instruction_selection_for_x86.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Extract Operand And Legality Selectors
Plan Review Counter: 1 / 10
# Current Packet

## Just Finished

Step 2 extracted one reusable prepared i32 value selector in
`prepared_local_slot_render.cpp` and reused it in the current guard/return
route. `render_prepared_local_slot_guard_chain_if_supported` now routes its
i32 global/local store sources and return value through the shared selector,
and `render_prepared_bounded_same_module_helper_prefix_if_supported` now
reuses the same selector for its `eax` setup, bare operand rendering,
global-store source selection, and i32 return emission instead of open-coding
those value-kind checks.

## Suggested Next

Stay in Step 2 and extract the next legality-focused selector only where the
guard-chain path still open-codes current versus previous materialized i32
carrier selection, without widening into arithmetic or multi-defined call-lane
routes.

## Watchouts

- Do not add new whole-function or named-testcase x86 matcher families.
- Consume prepared CFG, value-home, move-bundle, and addressing facts instead
  of re-deriving them from local emitter shape.
- Keep idea 59 focused on scalar instruction selection structure, not on
  reopening upstream ownership from ideas 58, 60, or 61.
- Prefer one coherent instruction-family migration per packet over broad
  emitter rewrites.
- The new helper only covers prepared i32 immediate-versus-named selection for
  the current guard/return route. Previous-value (`ecx`) legality, compare
  planning, and broader family migration are still separate Step 2 or Step 3
  work.
- The matching `^backend_` before/after logs are not fully green: both
  `test_before.log` and `test_after.log` fail in
  `backend_codegen_route_x86_64_variadic_double_bytes_observe_semantic_bir`,
  `backend_codegen_route_x86_64_variadic_pair_second_observe_semantic_bir`,
  `backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir`,
  and `backend_codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir`.

## Proof

Ran the delegated proof command `cmake --build --preset default && ctest
--test-dir build -j --output-on-failure -R '^backend_'` and captured the
output in `test_after.log`. The build completed successfully after this Step 2
i32 value-selector extraction, and the `^backend_` subset still matches the
current `test_before.log` baseline with these same four failing tests:
`backend_codegen_route_x86_64_variadic_double_bytes_observe_semantic_bir`,
`backend_codegen_route_x86_64_variadic_pair_second_observe_semantic_bir`,
`backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir`,
and `backend_codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir`.
