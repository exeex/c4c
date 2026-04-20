# Execution State

Status: Active
Source Idea Path: ideas/open/59_generic_scalar_instruction_selection_for_x86.md
Source Plan Path: plan.md
Current Step ID: 3.4
Current Step Title: Audit Remaining Scalar Family Seams And Hand Off
Plan Review Counter: 0 / 10
# Current Packet

## Just Finished

Step 3.3 extracted the bounded same-module helper lane in
`prepared_local_slot_render.cpp` into a named per-inst dispatcher,
`render_supported_helper_inst`, so the helper-function loop now routes its
covered scalar binary and same-module `i32` store families through one local
instruction-selection surface instead of open-coding those families inline.

## Suggested Next

Execute Step 3.4 by auditing `prepared_local_slot_render.cpp` for any real
remaining covered scalar instruction-family seam; if no such seam remains,
close out Step 3 and move the next packet to Step 4 call or terminator
migration instead of forcing another helper-only refactor.

## Watchouts

- Do not add new whole-function or named-testcase x86 matcher families.
- Consume prepared CFG, value-home, move-bundle, and addressing facts instead
  of re-deriving them from local emitter shape.
- Keep idea 59 focused on scalar instruction selection structure, not on
  reopening upstream ownership from ideas 58, 60, or 61.
- Prefer one coherent instruction-family migration per packet over broad
  emitter rewrites.
- The new same-module helper dispatcher is intentionally limited to the
  already-covered scalar binary and same-module `i32` store families inside
  the bounded helper-function lane; do not widen it into call-lane, terminator,
  or unrelated whole-block ownership.
- The scalar load/store helpers and the bounded helper dispatcher now form the
  main Step 3 per-op surfaces in this file; any follow-up should prove a real
  uncovered scalar family remains before adding another extraction packet.
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
Step 3 bounded same-module helper dispatcher slice. The final `^backend_`
subset in `test_after.log` matched the accepted `test_before.log` failure set
exactly:
`backend_codegen_route_x86_64_variadic_double_bytes_observe_semantic_bir`,
`backend_codegen_route_x86_64_variadic_pair_second_observe_semantic_bir`,
`backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir`,
and `backend_codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir`.
