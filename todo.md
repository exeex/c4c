# Execution State

Status: Active
Source Idea Path: ideas/open/59_generic_scalar_instruction_selection_for_x86.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Extract Operand And Legality Selectors
Plan Review Counter: 4 / 10
# Current Packet

## Just Finished

Step 2 extracted the next legality-focused guard-chain helper in
`prepared_local_slot_render.cpp` by factoring the current `eax` plus immediate
`Add/Sub` lane into
`select_prepared_i32_eax_immediate_binary_if_supported`. The materialized
guard-chain local route now reuses that selector instead of open-coding the
current-value/immediate legality check inline before emitting the `mov ecx,
eax` plus arithmetic sequence.

## Suggested Next

Stay in Step 2 and extract the next legality selector that removes another
guard-chain-local operand or call-lane seam, but do not widen yet into
broader arithmetic-family migration, multi-defined call routing, or Step 3
per-op dispatch.

## Watchouts

- Do not add new whole-function or named-testcase x86 matcher families.
- Consume prepared CFG, value-home, move-bundle, and addressing facts instead
  of re-deriving them from local emitter shape.
- Keep idea 59 focused on scalar instruction selection structure, not on
  reopening upstream ownership from ideas 58, 60, or 61.
- Prefer one coherent instruction-family migration per packet over broad
  emitter rewrites.
- The new helper only covers the current materialized `eax` plus immediate
  `Add/Sub` seam in the guard-chain local route. Broader arithmetic-family
  migration and compare planning are still separate Step 2 or Step 3 work.
- The matching `^backend_` before/after logs are not fully green: both
  `test_before.log` and `test_after.log` fail in
  `backend_codegen_route_x86_64_variadic_double_bytes_observe_semantic_bir`,
  `backend_codegen_route_x86_64_variadic_pair_second_observe_semantic_bir`,
  `backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir`,
  and `backend_codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir`.

## Proof

Ran the proof command `cmake --build --preset default && ctest --test-dir
build -j --output-on-failure -R '^backend_'` and captured the output in
`test_after.log`. The build completed successfully after this Step 2
`eax`-immediate arithmetic selector extraction, and the `^backend_` subset
kept the same four failing tests already present in `test_before.log`:
`backend_codegen_route_x86_64_variadic_double_bytes_observe_semantic_bir`,
`backend_codegen_route_x86_64_variadic_pair_second_observe_semantic_bir`,
`backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir`,
and `backend_codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir`.
