# Execution State

Status: Active
Source Idea Path: ideas/open/59_generic_scalar_instruction_selection_for_x86.md
Source Plan Path: plan.md
Current Step ID: 2.2
Current Step Title: Consolidate Shared Scalar Operand And Compare Selectors
Plan Review Counter: 3 / 10
# Current Packet

## Just Finished

Step 2.2 extended the reusable prepared named-vs-immediate `i32` compare
surface in `prepared_local_slot_render.cpp` with filtered selectors for one
expected compared value and for a candidate compared-value set, then rewired
the guard-chain materialized-compare setup plus the raw and prepared local
guard immediate-branch planners to consume that shared compared-value matching
contract instead of re-checking names ad hoc.

## Suggested Next

Continue Step 2.2 by deciding whether the bounded guard-chain compare-context
builders in `prepared_param_zero_render.cpp` should consume the same filtered
named-immediate compare selector contract, so the remaining compare-planning
surface is shared across the nearby guarded branch routes before Step 2.3
call-lane legality work begins.

## Watchouts

- Do not add new whole-function or named-testcase x86 matcher families.
- Consume prepared CFG, value-home, move-bundle, and addressing facts instead
  of re-deriving them from local emitter shape.
- Keep idea 59 focused on scalar instruction selection structure, not on
  reopening upstream ownership from ideas 58, 60, or 61.
- Prefer one coherent instruction-family migration per packet over broad
  emitter rewrites.
- The new filtered compare helpers stay local to
  `prepared_local_slot_render.cpp`; the compare-context builders in
  `prepared_param_zero_render.cpp` still own their own false-branch opcode
  selection and have not been rewired to this contract yet.
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
filtered compare-selector extraction, and the `^backend_` subset kept the same
four failing tests already present in `test_before.log`:
`backend_codegen_route_x86_64_variadic_double_bytes_observe_semantic_bir`,
`backend_codegen_route_x86_64_variadic_pair_second_observe_semantic_bir`,
`backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir`,
and `backend_codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir`.
