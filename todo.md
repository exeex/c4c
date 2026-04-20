# Execution State

Status: Active
Source Idea Path: ideas/open/59_generic_scalar_instruction_selection_for_x86.md
Source Plan Path: plan.md
Current Step ID: 4.3
Current Step Title: Audit The Bounded Multi-Defined Call Lane
Plan Review Counter: 1 / 10
# Current Packet

## Just Finished

Step 4.3 moved one real covered per-call seam out of the bounded
multi-defined call lane in `x86_codegen.hpp`: the body matcher now routes
post-call result-carrier finalization through
`finalize_prepared_bounded_multi_defined_call_result_if_supported` instead
of open-coding that ABI result handoff inline inside
`render_prepared_bounded_multi_defined_call_lane_body_if_supported`.

## Suggested Next

Keep Step 4.3 in `x86_codegen.hpp`, but limit the next packet to a short
audit of whether the bounded multi-defined call lane still owns one more real
covered seam near return finalization or call-argument selection; if the
rest is only call spelling and frame/data glue, mark the lane structurally
exhausted and hand the next packet to the next Step 4 substep instead of
forcing another thin extraction.

## Watchouts

- Do not add new whole-function or named-testcase x86 matcher families.
- Consume prepared CFG, value-home, move-bundle, and addressing facts instead
  of re-deriving them from local emitter shape.
- Keep idea 59 focused on scalar instruction selection structure, not on
  reopening upstream ownership from ideas 58, 60, or 61.
- Prefer one coherent instruction-family migration per packet over broad
  emitter rewrites.
- Step 4.3 is implemented in `src/backend/mir/x86/codegen/x86_codegen.hpp`,
  not in `prepared_local_slot_render.cpp`; keep the next packet scoped to the
  header body matcher and avoid bouncing ownership back to the `.cpp`.
- The bounded multi-defined call lane now has an explicit post-call result
  finalizer; the next packet should confirm that any further extraction is a
  real per-call or per-terminator seam, not just call spelling or frame
  wrapper glue.
- The same-module helper-call lane remains exhausted for Step 4; stay on the
  bounded multi-defined call lane until it is either exhausted or yields one
  more real seam.
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
Step 4.3 bounded multi-defined call-result helper extraction. The final
`^backend_` subset in `test_after.log` preserved the accepted
`test_before.log` failure set exactly, with no new failures:
`backend_codegen_route_x86_64_variadic_double_bytes_observe_semantic_bir`,
`backend_codegen_route_x86_64_variadic_pair_second_observe_semantic_bir`,
`backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir`,
and `backend_codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir`.
