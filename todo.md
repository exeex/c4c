# Execution State

Status: Active
Source Idea Path: ideas/open/59_generic_scalar_instruction_selection_for_x86.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Establish Prepared Dispatch Surface
Plan Review Counter: 5 / 10
# Current Packet

## Just Finished

Step 1 kept the prepared x86 route structural by moving
`render_prepared_single_block_return_dispatch_if_supported` onto a
`PreparedX86FunctionDispatchContext` overload. The single-block return lane now
consumes the prepared function-dispatch surface from `prepared_module_emit.cpp`
instead of publishing another long raw-argument helper seam for module,
prepared metadata, return register, and helper callbacks.

## Suggested Next

If Step 1 stays open, review whether any remaining prepared x86 structural
helpers still publish raw function or block argument bundles that should
collapse onto the dispatch-context surface before widening into Step 2 selector
extraction.

## Watchouts

- Do not add new whole-function or named-testcase x86 matcher families.
- Consume prepared CFG, value-home, move-bundle, and addressing facts instead
  of re-deriving them from local emitter shape.
- Keep idea 59 focused on scalar instruction selection structure, not on
  reopening upstream ownership from ideas 58, 60, or 61.
- Prefer one coherent instruction-family migration per packet over broad
  emitter rewrites.
- `render_prepared_single_block_return_dispatch_if_supported` now uses the
  function-dispatch context, but other structural helper seams may still expose
  raw bundles; keep Step 1 structural and avoid widening into selector logic
  until those seams are reviewed.
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
