# Execution State

Status: Active
Source Idea Path: ideas/open/59_generic_scalar_instruction_selection_for_x86.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Establish Prepared Dispatch Surface
Plan Review Counter: 8 / 10
# Current Packet

## Just Finished

Step 1 kept the prepared x86 route structural by moving
`render_prepared_single_block_return_dispatch_if_supported` onto
`PreparedX86FunctionDispatchContext` end to end. The active single-block return
dispatch path now owns its routing logic directly from the function-dispatch
context instead of immediately unpacking that context back into a long raw
argument bundle.

## Suggested Next

Re-check whether any other Step 1 helpers in the active prepared x86 scalar
route still unwrap `PreparedX86FunctionDispatchContext` into long raw argument
bundles. If this was the last real structural seam, close Step 1 and route the
next packet into Step 2 selector extraction rather than inventing more
dispatch-surface churn.

## Watchouts

- Do not add new whole-function or named-testcase x86 matcher families.
- Consume prepared CFG, value-home, move-bundle, and addressing facts instead
  of re-deriving them from local emitter shape.
- Keep idea 59 focused on scalar instruction selection structure, not on
  reopening upstream ownership from ideas 58, 60, or 61.
- Prefer one coherent instruction-family migration per packet over broad
  emitter rewrites.
- `render_prepared_single_block_return_dispatch_if_supported` now consumes the
  function-dispatch context directly, so Step 1 should only stay open if a real
  remaining structural seam still unwraps that context in the active scalar
  path.
- The matching `^backend_` before/after logs are not fully green: both
  `test_before.log` and `test_after.log` fail in
  `backend_codegen_route_x86_64_variadic_double_bytes_observe_semantic_bir`,
  `backend_codegen_route_x86_64_variadic_pair_second_observe_semantic_bir`,
  `backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir`,
  and `backend_codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir`.

## Proof

Ran the delegated proof command `cmake --build --preset default && ctest
--test-dir build -j --output-on-failure -R '^backend_'` and captured the
output in `test_after.log`. The build completed successfully after the
single-block return dispatch-context refactor, and the `^backend_` subset is
not fully green because `test_before.log` and `test_after.log` still share the
same four failing tests:
`backend_codegen_route_x86_64_variadic_double_bytes_observe_semantic_bir`,
`backend_codegen_route_x86_64_variadic_pair_second_observe_semantic_bir`,
`backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir`,
and `backend_codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir`.
