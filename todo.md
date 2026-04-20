# Execution State

Status: Active
Source Idea Path: ideas/open/59_generic_scalar_instruction_selection_for_x86.md
Source Plan Path: plan.md
Current Step ID: 4.3
Current Step Title: Audit The Bounded Multi-Defined Call Lane
Plan Review Counter: 0 / 10
# Current Packet

## Just Finished

Step 4 audited the covered same-module helper-call lane in
`prepared_local_slot_render.cpp` after the recent selector/state extractions
and judged it structurally exhausted: the remaining `xor eax` / `call`
spelling in `render_prepared_bounded_same_module_helper_call_if_supported`
and the thin block-loop wrapper in
`render_prepared_block_same_module_helper_call_inst_if_supported` are route
glue, not another real per-call dispatch seam worth extracting.

## Suggested Next

Move the next Step 4 packet to the bounded multi-defined call lane in
`prepared_local_slot_render.cpp`, starting with a short audit of whether
`render_prepared_bounded_multi_defined_call_lane_body_if_supported` still
owns one real covered per-call or per-terminator seam that should become
explicit helper selection instead of staying inline inside that body matcher.

## Watchouts

- Do not add new whole-function or named-testcase x86 matcher families.
- Consume prepared CFG, value-home, move-bundle, and addressing facts instead
  of re-deriving them from local emitter shape.
- Keep idea 59 focused on scalar instruction selection structure, not on
  reopening upstream ownership from ideas 58, 60, or 61.
- Prefer one coherent instruction-family migration per packet over broad
  emitter rewrites.
- The same-module helper-call lane is now structurally exhausted for Step 4:
  its remaining call spelling and block-loop handoff are wrapper glue, not
  the next selector target.
- Do not reopen the same-module helper-call lane with more thin extractions
  unless a later packet finds a genuinely new legality or dispatch seam.
- The next packet should stay on covered call/terminator migration by moving
  to the bounded multi-defined call lane, not back into the exhausted direct
  extern or same-module helper-call families.
- Step 4 should keep moving the active route toward prepared per-call or
  per-terminator selection, not back into Step 3 scalar family cleanup or
  into broad single-block fallback rewrites.
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
Step 4 same-module helper-call exhaustion audit and handoff. The final
`^backend_` subset in `test_after.log` preserved the accepted
`test_before.log` failure set exactly, with no new failures:
`backend_codegen_route_x86_64_variadic_double_bytes_observe_semantic_bir`,
`backend_codegen_route_x86_64_variadic_pair_second_observe_semantic_bir`,
`backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir`,
and `backend_codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir`.
