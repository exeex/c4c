# Execution State

Status: Active
Source Idea Path: ideas/open/59_generic_scalar_instruction_selection_for_x86.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Extract Operand And Legality Selectors
Plan Review Counter: 10 / 10
# Current Packet

## Just Finished

Step 2 extracted the remaining inline local `i32` guard candidate-expression
collection seam in `prepared_local_slot_render.cpp` by factoring the
pre-compare store/load/binary scan into
`select_prepared_local_i32_guard_expression_surface_if_supported`. The local
`i32` arithmetic guard now consumes one helper-returned setup-asm plus named
expression/candidate-compare surface instead of assembling that selector state
inline before branch-plan selection.

## Suggested Next

Stay in Step 2 and audit whether the nearby local `i32` guard operand/value
render recursion still hides one reusable selector seam, or whether this guard
surface is now structurally exhausted and the next packet should move to a
different Step 2 selector surface, but do not widen into broader arithmetic
family migration, prepared move-bundle call routing, or Step 3 per-op
dispatch.

## Watchouts

- Do not add new whole-function or named-testcase x86 matcher families.
- Consume prepared CFG, value-home, move-bundle, and addressing facts instead
  of re-deriving them from local emitter shape.
- Keep idea 59 focused on scalar instruction selection structure, not on
  reopening upstream ownership from ideas 58, 60, or 61.
- Prefer one coherent instruction-family migration per packet over broad
  emitter rewrites.
- The local `i32` guard now gets its setup asm and candidate-expression surface
  from one helper, so any further Step 2 cleanup nearby should only happen if
  the remaining operand/value rendering logic can be extracted as a real shared
  selector surface rather than as cosmetic refactoring.
- The matching `^backend_` before/after logs are not fully green: both
  `test_before.log` and `test_after.log` fail in
  `backend_codegen_route_x86_64_variadic_double_bytes_observe_semantic_bir`,
  `backend_codegen_route_x86_64_variadic_pair_second_observe_semantic_bir`,
  `backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir`,
  and `backend_codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir`.

## Proof

Ran the proof command `cmake --build --preset default && ctest --test-dir
build -j --output-on-failure -R '^backend_'` and captured the output in
`test_after.log`. The build completed successfully after this Step 2 local
`i32` guard expression-surface helper extraction, and the `^backend_` subset
kept the same four failing tests already present in `test_before.log`:
`backend_codegen_route_x86_64_variadic_double_bytes_observe_semantic_bir`,
`backend_codegen_route_x86_64_variadic_pair_second_observe_semantic_bir`,
`backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir`,
and `backend_codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir`.
