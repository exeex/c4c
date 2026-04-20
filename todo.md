# Execution State

Status: Active
Source Idea Path: ideas/open/59_generic_scalar_instruction_selection_for_x86.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Migrate Covered Scalar Instruction Families
Plan Review Counter: 1 / 10
# Current Packet

## Just Finished

Step 3 moved the bounded same-module helper route's covered `i32`
store-global lane onto a shared prepared store renderer in
`x86_codegen.hpp`, and rewired both the helper-local same-module store path
and the existing prepared local scalar store path in
`prepared_local_slot_render.cpp` to consume that selector surface instead of
spelling the final store emission inline.

## Suggested Next

Keep Step 3 bounded to one adjacent scalar family by auditing whether the
same-module helper route's covered load-global lane should now move onto a
shared prepared load selector surface; if that does not yield a real reusable
helper, choose another single covered scalar family rather than widening the
packet.

## Watchouts

- Do not add new whole-function or named-testcase x86 matcher families.
- Consume prepared CFG, value-home, move-bundle, and addressing facts instead
  of re-deriving them from local emitter shape.
- Keep idea 59 focused on scalar instruction selection structure, not on
  reopening upstream ownership from ideas 58, 60, or 61.
- Prefer one coherent instruction-family migration per packet over broad
  emitter rewrites.
- The new shared store helper still relies on caller-supplied `i32` operand
  selectors; keep param-register lookup, previous-value tracking, and
  same-module global ownership local until another real consumer needs a
  wider contract.
- The matching `^backend_` before/after logs are not fully green: both
  `test_before.log` and `test_after.log` fail in
  `backend_codegen_route_x86_64_variadic_double_bytes_observe_semantic_bir`,
  `backend_codegen_route_x86_64_variadic_pair_second_observe_semantic_bir`,
  `backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir`,
  and `backend_codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir`.

## Proof

Ran the proof command `cmake --build --preset default && ctest --test-dir
build -j --output-on-failure -R '^backend_' 2>&1 | tee
/workspaces/c4c/test_after.log`. The build completed successfully after this
Step 3 bounded same-module helper store-family slice, and the `^backend_`
subset kept the same four failing tests already present in `test_before.log`:
`backend_codegen_route_x86_64_variadic_double_bytes_observe_semantic_bir`,
`backend_codegen_route_x86_64_variadic_pair_second_observe_semantic_bir`,
`backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir`,
and `backend_codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir`.
