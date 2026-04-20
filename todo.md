# Execution State

Status: Active
Source Idea Path: ideas/open/59_generic_scalar_instruction_selection_for_x86.md
Source Plan Path: plan.md
Current Step ID: 1.3
Current Step Title: Audit Remaining Dispatch Seams And Hand Off To Selector Work
Plan Review Counter: 1 / 10
# Current Packet

## Just Finished

Step 1.3 audited the remaining prepared-x86 structural dispatch seams and
collapsed the single-block return family onto context-native helpers. The
active return dispatch no longer unwraps `PreparedX86FunctionDispatchContext`
back into long raw helper argument bundles, and the remaining Step 1 entry and
guard helpers in the active scalar path already consume the shared context
directly.

## Suggested Next

Move into Step 2 selector extraction. Start with one bounded helper surface for
prepared scalar operand legality, such as extracting reusable stack/symbol
memory operand or named/immediate value selection from the current return and
guard helpers without widening into whole-family migration.

## Watchouts

- Do not add new whole-function or named-testcase x86 matcher families.
- Consume prepared CFG, value-home, move-bundle, and addressing facts instead
  of re-deriving them from local emitter shape.
- Keep idea 59 focused on scalar instruction selection structure, not on
  reopening upstream ownership from ideas 58, 60, or 61.
- Prefer one coherent instruction-family migration per packet over broad
  emitter rewrites.
- `render_prepared_single_block_return_dispatch_if_supported` now stays on
  context-native helpers through the whole active return family, and the
  remaining Step 1 entry/guard helpers were already consuming the shared
  context, so additional Step 1 work would be route drift unless a new
  function-wide raw seam is found.
- The matching `^backend_` before/after logs are not fully green: both
  `test_before.log` and `test_after.log` fail in
  `backend_codegen_route_x86_64_variadic_double_bytes_observe_semantic_bir`,
  `backend_codegen_route_x86_64_variadic_pair_second_observe_semantic_bir`,
  `backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir`,
  and `backend_codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir`.

## Proof

Ran the delegated proof command `cmake --build --preset default && ctest
--test-dir build -j --output-on-failure -R '^backend_'` and captured the
output in `test_after.log`. The build completed successfully after the Step
1.3 return-family context refactor, and the `^backend_` subset still matches
the current `test_before.log` baseline with these same four failing tests:
`backend_codegen_route_x86_64_variadic_double_bytes_observe_semantic_bir`,
`backend_codegen_route_x86_64_variadic_pair_second_observe_semantic_bir`,
`backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir`,
and `backend_codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir`.
