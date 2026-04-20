# Execution State

Status: Active
Source Idea Path: ideas/open/59_generic_scalar_instruction_selection_for_x86.md
Source Plan Path: plan.md
Current Step ID: 2.3
Current Step Title: Extract Bounded Call-Lane Legality Selectors
Plan Review Counter: 0 / 10
# Current Packet

## Just Finished

Step 2.3 extracted shared prepared call-bundle legality selectors into
`x86_codegen.hpp` by centralizing `i32` argument ABI-register lookup, call-
result ABI-register lookup, and shared `narrow_i32_register` use, then
rewiring both bounded call renderers in `prepared_local_slot_render.cpp` to
consume that selector surface instead of repeating `BeforeCall` and
`AfterCall` bundle scans inline.

## Suggested Next

Audit whether any remaining prepared-x86 scalar call renderers still duplicate
authoritative call-bundle lookup outside the bounded direct-extern and multi-
defined call lanes; if no real second consumer remains, treat Step 2.3 as
structurally exhausted and move the next packet to Step 3 family migration.

## Watchouts

- Do not add new whole-function or named-testcase x86 matcher families.
- Consume prepared CFG, value-home, move-bundle, and addressing facts instead
  of re-deriving them from local emitter shape.
- Keep idea 59 focused on scalar instruction selection structure, not on
  reopening upstream ownership from ideas 58, 60, or 61.
- Prefer one coherent instruction-family migration per packet over broad
  emitter rewrites.
- The shared bounded call-bundle selectors now cross translation-unit
  boundaries through `x86_codegen.hpp`; keep call-source materialization and
  current-value carrier state local until another real consumer needs a wider
  helper contract.
- `prepared_param_zero_render.cpp` now relies on the shared
  `narrow_i32_register` helper from `x86_codegen.hpp`; avoid reintroducing a
  second local register-narrowing copy.
- The matching `^backend_` before/after logs are not fully green: both
  `test_before.log` and `test_after.log` fail in
  `backend_codegen_route_x86_64_variadic_double_bytes_observe_semantic_bir`,
  `backend_codegen_route_x86_64_variadic_pair_second_observe_semantic_bir`,
  `backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir`,
  and `backend_codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir`.

## Proof

Ran the proof command `cmake --build --preset default && ctest --test-dir
build -j --output-on-failure -R '^backend_'` and captured the output in
`test_after.log`. The build completed successfully after this Step 2.3
bounded call-lane legality-selector slice, and the `^backend_` subset kept the
same four failing tests already present in `test_before.log`:
`backend_codegen_route_x86_64_variadic_double_bytes_observe_semantic_bir`,
`backend_codegen_route_x86_64_variadic_pair_second_observe_semantic_bir`,
`backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir`,
and `backend_codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir`.
