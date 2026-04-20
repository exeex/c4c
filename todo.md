# Execution State

Status: Active
Source Idea Path: ideas/open/59_generic_scalar_instruction_selection_for_x86.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Extract Operand And Legality Selectors
Plan Review Counter: 3 / 10
# Current Packet

## Just Finished

Step 2 extracted the next legality-focused guard-chain helper in
`prepared_local_slot_render.cpp` by factoring the current local compare setup
into `select_prepared_materialized_i32_compare_setup_if_supported`. The
materialized `Eq/Ne` guard-chain path now reuses that selector instead of
open-coding the immediate-vs-current-EAX and immediate-vs-immediate compare
setup inline.

## Suggested Next

Stay in Step 2 and extract the next legality selector that removes another
guard-chain-local operand or compare-planning seam, but do not widen yet into
arithmetic-family migration, multi-defined call-lane routing, or broader Step
3 per-op dispatch.

## Watchouts

- Do not add new whole-function or named-testcase x86 matcher families.
- Consume prepared CFG, value-home, move-bundle, and addressing facts instead
  of re-deriving them from local emitter shape.
- Keep idea 59 focused on scalar instruction selection structure, not on
  reopening upstream ownership from ideas 58, 60, or 61.
- Prefer one coherent instruction-family migration per packet over broad
  emitter rewrites.
- The new helper only covers the current materialized `i32` compare setup seam
  in the guard-chain local route. Broader compare planning and instruction
  family migration are still separate Step 2 or Step 3 work.
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
compare-setup selector extraction, and the `^backend_` subset still matches
the current `test_before.log` baseline with these same four failing tests:
`backend_codegen_route_x86_64_variadic_double_bytes_observe_semantic_bir`,
`backend_codegen_route_x86_64_variadic_pair_second_observe_semantic_bir`,
`backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir`,
and `backend_codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir`.
