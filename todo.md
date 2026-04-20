# Execution State

Status: Active
Source Idea Path: ideas/open/59_generic_scalar_instruction_selection_for_x86.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Establish Prepared Dispatch Surface
Plan Review Counter: 1 / 10
# Current Packet

## Just Finished

Step 1 established an explicit prepared x86 dispatch surface by adding
`PreparedX86FunctionDispatchContext` and `PreparedX86BlockDispatchContext`,
wiring `emit_prepared_module` to build that function context, and routing the
local-slot guard-chain entry through the packaged prepared function/block
inputs instead of a long raw parameter list.

## Suggested Next

Keep Step 1 structural and continue extending the prepared x86 dispatch
surface in bounded packets without widening into instruction-family migration
or whole-function matcher growth.

## Watchouts

- Do not add new whole-function or named-testcase x86 matcher families.
- Consume prepared CFG, value-home, move-bundle, and addressing facts instead
  of re-deriving them from local emitter shape.
- Keep idea 59 focused on scalar instruction selection structure, not on
  reopening upstream ownership from ideas 58, 60, or 61.
- Prefer one coherent instruction-family migration per packet over broad
  emitter rewrites.
- The matching `^backend_` before/after logs are not fully green: both
  `test_before.log` and `test_after.log` fail in
  `backend_codegen_route_x86_64_variadic_double_bytes_observe_semantic_bir`,
  `backend_codegen_route_x86_64_variadic_pair_second_observe_semantic_bir`,
  `backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir`,
  and `backend_codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir`.

## Proof

Supervisor-side regression evidence now uses matching before/after runs of
`cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^backend_'`, with results captured in
`test_before.log` and `test_after.log`. Both logs show the same four failing
tests:
`backend_codegen_route_x86_64_variadic_double_bytes_observe_semantic_bir`,
`backend_codegen_route_x86_64_variadic_pair_second_observe_semantic_bir`,
`backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir`,
and `backend_codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir`.
The delegated backend subset is therefore non-regressive for this Step 1
structural slice, even though the subset is not fully green.
