# Execution State

Status: Active
Source Idea Path: ideas/open/59_generic_scalar_instruction_selection_for_x86.md
Source Plan Path: plan.md
Current Step ID: 4.4
Current Step Title: Finish Remaining Covered Branch And Residual Call Families
Plan Review Counter: 1 / 10
# Current Packet

## Just Finished

Step 4.4 moved the nearby covered `Branch` compare-join vs plain-branch
handoff out of the main block renderer in
`prepared_local_slot_render.cpp`: `render_prepared_local_slot_guard_chain_if_supported`
now delegates that per-terminator decision to
`select_prepared_block_branch_render_if_supported` and
`render_prepared_block_branch_terminator_if_supported` instead of open-coding
the compare-join render-plan build inline before falling back to the plain
branch path.

## Suggested Next

Treat this nearby branch-terminator helper lane in
`prepared_local_slot_render.cpp` as structurally exhausted for Step 4.4 unless
the next audit finds a genuinely new prepared per-terminator selector
contract; otherwise move the next packet to the remaining residual covered
Step 4.4 call or terminator route instead of forcing another thin branch-only
helper extraction.

## Watchouts

- Do not add new whole-function or named-testcase x86 matcher families.
- Consume prepared CFG, value-home, move-bundle, and addressing facts instead
  of re-deriving them from local emitter shape.
- Keep idea 59 focused on scalar instruction selection structure, not on
  reopening upstream ownership from ideas 58, 60, or 61.
- Prefer one coherent instruction-family migration per packet over broad
  emitter rewrites.
- Step 4.4 genuinely lives in `prepared_local_slot_render.cpp` now: stay on the
  branch terminator helpers there instead of reopening the exhausted Step 4.3
  bounded multi-defined call lane in `x86_codegen.hpp`.
- The bounded multi-defined call lane remains structurally exhausted for Step
  4.3: its remaining body logic is callee admission, `call` spelling, and
  wrapper/data glue, not another real selector seam worth extracting.
- The same-module helper-call lane remains exhausted for Step 4; do not reopen
  either exhausted call lane unless later Step 4.4 branch or residual-call
  work exposes a genuinely new prepared selector contract.
- Step 4 should keep moving the active route toward prepared per-call or
  per-terminator selection, not back into Step 3 scalar family cleanup or
  into broad single-block fallback rewrites.
- The nearby branch compare-join/plain-branch handoff now has an explicit
  helper seam; do not spend another packet on that area unless a new
  authoritative prepared selector boundary appears beyond target recursion or
  final block-render glue.
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
Step 4.4 branch compare-join/plain-branch helper extraction. The final
`^backend_` subset in `test_after.log` preserved the accepted
`test_before.log` failure set exactly, with no new failures:
`backend_codegen_route_x86_64_variadic_double_bytes_observe_semantic_bir`,
`backend_codegen_route_x86_64_variadic_pair_second_observe_semantic_bir`,
`backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir`,
and `backend_codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir`.
