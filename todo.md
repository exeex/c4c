# Execution State

Status: Active
Source Idea Path: ideas/open/59_generic_scalar_instruction_selection_for_x86.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Migrate Covered Terminator And Call Families
Plan Review Counter: 4 / 10
# Current Packet

## Just Finished

Step 4 extracted the remaining covered conditional-branch terminator lane in
`prepared_local_slot_render.cpp` out of the inline `render_block` path and
into the named helper
`render_prepared_block_cond_branch_terminator_if_supported`, so compare-driven
cond-branch dispatch now routes through one dedicated helper instead of
keeping the short-circuit handoff checks and plain cond-plan rendering
open-coded in the block renderer.

## Suggested Next

Keep Step 4 bounded to the remaining covered call-family seams in
`prepared_local_slot_render.cpp`, so one coherent prepared helper-call or
call-result dispatch lane moves out of inline `render_block` without widening
into variadic, external-call, or branch-plan rewrites.

## Watchouts

- Do not add new whole-function or named-testcase x86 matcher families.
- Consume prepared CFG, value-home, move-bundle, and addressing facts instead
  of re-deriving them from local emitter shape.
- Keep idea 59 focused on scalar instruction selection structure, not on
  reopening upstream ownership from ideas 58, 60, or 61.
- Prefer one coherent instruction-family migration per packet over broad
  emitter rewrites.
- The new helper is intentionally bounded to the covered cond-branch
  terminator lane in `render_block`; do not widen it into indirect-call,
  variadic, external-call, or broader compare-plan semantic rewrites in the
  same packet.
- Step 4 should keep moving the active route toward prepared per-call or
  per-terminator selection in `render_block`, not back into Step 3 scalar
  family cleanup or broader fallback-family rewrites.
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
Step 4 conditional-branch helper extraction. The final `^backend_` subset in
`test_after.log` preserved the accepted `test_before.log` failure set
exactly, with no new failures:
`backend_codegen_route_x86_64_variadic_double_bytes_observe_semantic_bir`,
`backend_codegen_route_x86_64_variadic_pair_second_observe_semantic_bir`,
`backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir`,
and `backend_codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir`.
