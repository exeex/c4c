# Execution State

Status: Active
Source Idea Path: ideas/open/59_generic_scalar_instruction_selection_for_x86.md
Source Plan Path: plan.md
Current Step ID: 2.2
Current Step Title: Consolidate Shared Scalar Operand And Compare Selectors
Plan Review Counter: 0 / 10
# Current Packet

## Just Finished

Step 2.2 extracted reusable prepared `i32` operand and register-move selector
entry points in `prepared_local_slot_render.cpp` so the bounded helper and
call/operand routes no longer reassemble `PreparedI32ValueSelection` into asm
strings by hand. The packet added shared helpers for rendering a prepared `i32`
as an operand or as a move into a target register, then rewired the existing
call-argument, bounded-helper binary, global-store, and return paths to consume
that selector surface.

## Suggested Next

Continue Step 2.2 with the next shared selector seam outside these `i32`
operand helpers, preferably by consolidating the prepared same-module global
scalar memory legality/selection checks that still appear ad hoc across load,
store, and bounded-helper paths, but do not widen into broader arithmetic
family migration, prepared move-bundle call routing, or Step 3 per-op dispatch.

## Watchouts

- Do not add new whole-function or named-testcase x86 matcher families.
- Consume prepared CFG, value-home, move-bundle, and addressing facts instead
  of re-deriving them from local emitter shape.
- Keep idea 59 focused on scalar instruction selection structure, not on
  reopening upstream ownership from ideas 58, 60, or 61.
- Prefer one coherent instruction-family migration per packet over broad
  emitter rewrites.
- The new helpers are intentionally limited to prepared `i32` operand/register
  selection; keep nearby `current_i32_name` and compare-materialization state
  transitions explicit until there is a broader selector contract for them.
- The next selector packet should target shared memory or compare legality with
  reuse beyond one bounded route, not just more cosmetic rewiring around these
  new operand helpers.
- The matching `^backend_` before/after logs are not fully green: both
  `test_before.log` and `test_after.log` fail in
  `backend_codegen_route_x86_64_variadic_double_bytes_observe_semantic_bir`,
  `backend_codegen_route_x86_64_variadic_pair_second_observe_semantic_bir`,
  `backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir`,
  and `backend_codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir`.

## Proof

Ran the proof command `cmake --build --preset default && ctest --test-dir
build -j --output-on-failure -R '^backend_'` and captured the output in
`test_after.log`. The build completed successfully after this Step 2.2 prepared
`i32` operand/move selector extraction, and the `^backend_` subset kept the
same four failing tests already present in `test_before.log`:
`backend_codegen_route_x86_64_variadic_double_bytes_observe_semantic_bir`,
`backend_codegen_route_x86_64_variadic_pair_second_observe_semantic_bir`,
`backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir`,
and `backend_codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir`.
