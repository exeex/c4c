# Execution State

Status: Active
Source Idea Path: ideas/open/59_generic_scalar_instruction_selection_for_x86.md
Source Plan Path: plan.md
Current Step ID: 2.1
Current Step Title: Exhaust The Remaining Local Guard Selector Seams
Plan Review Counter: 1 / 10
# Current Packet

## Just Finished

Step 2.1 audited the remaining local `i32` guard operand/value rendering
recursion in `prepared_local_slot_render.cpp` and judged the nearby seam
structurally exhausted. The recursion is still specific to this guard-only
named-expression tree and did not support a clear reusable prepared-legality
selector contract, so this packet did not force another cosmetic helper
extraction.

## Suggested Next

Move to Step 2.2 and extract the next real selector surface outside the local
guard audit, preferably a helper that answers prepared scalar operand or
legality questions with a contract broader than this one guard-specific
expression tree, but do not widen into broader arithmetic family migration,
prepared move-bundle call routing, or Step 3 per-op dispatch.

## Watchouts

- Do not add new whole-function or named-testcase x86 matcher families.
- Consume prepared CFG, value-home, move-bundle, and addressing facts instead
  of re-deriving them from local emitter shape.
- Keep idea 59 focused on scalar instruction selection structure, not on
  reopening upstream ownership from ideas 58, 60, or 61.
- Prefer one coherent instruction-family migration per packet over broad
  emitter rewrites.
- Step 2.1 should be treated as exhausted unless a later packet discovers a
  genuinely shared selector contract for the guard recursion; do not reopen
  this seam just to move code around.
- The next selector packet should target a helper surface with reuse beyond the
  local `i32` guard recursion.
- The matching `^backend_` before/after logs are not fully green: both
  `test_before.log` and `test_after.log` fail in
  `backend_codegen_route_x86_64_variadic_double_bytes_observe_semantic_bir`,
  `backend_codegen_route_x86_64_variadic_pair_second_observe_semantic_bir`,
  `backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir`,
  and `backend_codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir`.

## Proof

Ran the proof command `cmake --build --preset default && ctest --test-dir
build -j --output-on-failure -R '^backend_'` and captured the output in
`test_after.log`. The build completed successfully after this Step 2.1 local
guard selector-exhaustion audit, and the `^backend_` subset kept the same four
failing tests already present in `test_before.log`:
`backend_codegen_route_x86_64_variadic_double_bytes_observe_semantic_bir`,
`backend_codegen_route_x86_64_variadic_pair_second_observe_semantic_bir`,
`backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir`,
and `backend_codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir`.
