# Execution State

Status: Active
Source Idea Path: ideas/open/59_generic_scalar_instruction_selection_for_x86.md
Source Plan Path: plan.md
Current Step ID: 2.2
Current Step Title: Consolidate Shared Scalar Operand And Compare Selectors
Plan Review Counter: 4 / 10
# Current Packet

## Just Finished

Step 2.2 extended the shared prepared named-vs-immediate `i32` compare
selector contract into `prepared_param_zero_render.cpp` by exposing the
filtered compare-for-one-value helper plus the shared EAX compare-setup
renderer through `x86_codegen.hpp`, then rewiring the param-zero guard false-
branch compare builder to consume that selector surface instead of repeating
named-vs-immediate checks inline.

## Suggested Next

Decide whether any remaining guarded-branch compare planners still duplicate
named-vs-immediate or candidate-value compare filtering beyond the now-shared
param-zero and local-slot routes; if not, treat Step 2.2 as structurally
exhausted and move the next packet to Step 2.3 bounded call-lane legality
selectors.

## Watchouts

- Do not add new whole-function or named-testcase x86 matcher families.
- Consume prepared CFG, value-home, move-bundle, and addressing facts instead
  of re-deriving them from local emitter shape.
- Keep idea 59 focused on scalar instruction selection structure, not on
  reopening upstream ownership from ideas 58, 60, or 61.
- Prefer one coherent instruction-family migration per packet over broad
  emitter rewrites.
- The shared filtered compare-for-value helper and EAX compare-setup renderer
  now cross translation-unit boundaries through `x86_codegen.hpp`; keep the
  broader candidate-set helper local until a real second consumer appears.
- Keep nearby `current_i32_name`, `previous_i32_name`, and compare-materialized
  state transitions explicit until there is a genuinely reusable compared-value
  or register-state selector contract instead of cosmetic rewiring.
- The matching `^backend_` before/after logs are not fully green: both
  `test_before.log` and `test_after.log` fail in
  `backend_codegen_route_x86_64_variadic_double_bytes_observe_semantic_bir`,
  `backend_codegen_route_x86_64_variadic_pair_second_observe_semantic_bir`,
  `backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir`,
  and `backend_codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir`.

## Proof

Ran the proof command `cmake --build --preset default && ctest --test-dir
build -j --output-on-failure -R '^backend_'` and captured the output in
`test_after.log`. The build completed successfully after this Step 2.2
param-zero compare-selector reuse slice, and the `^backend_` subset kept the
same four failing tests already present in `test_before.log`:
`backend_codegen_route_x86_64_variadic_double_bytes_observe_semantic_bir`,
`backend_codegen_route_x86_64_variadic_pair_second_observe_semantic_bir`,
`backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir`,
and `backend_codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir`.
