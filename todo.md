# Execution State

Status: Active
Source Idea Path: ideas/open/59_generic_scalar_instruction_selection_for_x86.md
Source Plan Path: plan.md
Current Step ID: 4.3
Current Step Title: Audit The Bounded Multi-Defined Call Lane
Plan Review Counter: 3 / 10
# Current Packet

## Just Finished

Step 4.3 moved the last real covered per-terminator seam out of the bounded
multi-defined call lane in `x86_codegen.hpp`: the body matcher now routes the
bounded lane's immediate `i32` return finalization through
`finalize_prepared_bounded_multi_defined_return_if_supported` instead of
open-coding the return-register handoff, frame teardown, and `ret` sequence
inline inside `render_prepared_bounded_multi_defined_call_lane_body_if_supported`.

## Suggested Next

Move the next packet to Step 4.4. After the bounded multi-defined call lane's
argument, result, and return seams are explicit helpers, the remaining code in
that lane is callee admission, call spelling, and module/data wrapper glue, so
the next coherent work item is the remaining covered branch and residual
call-family migration rather than another thin Step 4.3 extraction.

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
  header body matcher and avoid bouncing ownership back to the `.cpp` unless
  Step 4.4 genuinely needs it.
- The bounded multi-defined call lane is now structurally exhausted for Step
  4.3: its remaining body logic is callee admission, `call` spelling, and
  wrapper/data glue, not another real selector seam worth extracting.
- The same-module helper-call lane remains exhausted for Step 4; do not reopen
  either exhausted call lane unless later Step 4.4 branch or residual-call
  work exposes a genuinely new prepared selector contract.
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
Step 4.3 bounded multi-defined return-finalization helper extraction. The
final `^backend_` subset in `test_after.log` preserved the accepted
`test_before.log` failure set exactly, with no new failures:
`backend_codegen_route_x86_64_variadic_double_bytes_observe_semantic_bir`,
`backend_codegen_route_x86_64_variadic_pair_second_observe_semantic_bir`,
`backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir`,
and `backend_codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir`.
