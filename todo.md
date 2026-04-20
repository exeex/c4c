# Execution State

Status: Active
Source Idea Path: ideas/open/59_generic_scalar_instruction_selection_for_x86.md
Source Plan Path: plan.md
Current Step ID: 4.4
Current Step Title: Finish Remaining Covered Branch And Residual Call Families
Plan Review Counter: 4 / 10
# Current Packet

## Just Finished

Step 4.4 split the remaining conditional-branch render selection seam in
`prepared_local_slot_render.cpp` into explicit selector helpers:
`select_prepared_short_circuit_cond_branch_render_if_supported` now owns the
authoritative prepared short-circuit entry selection, while
`select_prepared_plain_cond_branch_render_if_supported` owns the plain
prepared conditional-branch render plan before
`select_prepared_block_cond_branch_render_if_supported` hands the chosen plan
to the shared branch renderer.

## Suggested Next

Treat the conditional-branch selector split as structurally exhausted for Step
4.4 unless a later audit finds a genuinely new prepared per-terminator
contract beyond short-circuit-vs-plain render selection; otherwise use the
next packet to audit whether any real residual Step 4.4 branch or call seam
remains outside final block-render glue instead of reopening these helpers.

## Watchouts

- Do not add new whole-function or named-testcase x86 matcher families.
- Consume prepared CFG, value-home, move-bundle, and addressing facts instead
  of re-deriving them from local emitter shape.
- Keep idea 59 focused on scalar instruction selection structure, not on
  reopening upstream ownership from ideas 58, 60, or 61.
- Prefer one coherent instruction-family migration per packet over broad
  emitter rewrites.
- Step 4.4 still lives in `prepared_local_slot_render.cpp`; do not reopen the
  exhausted Step 4.3 bounded multi-defined call lane or the exhausted
  same-module helper-call lane unless later work exposes a genuinely new
  prepared selector contract.
- The conditional-branch route now has explicit selector helpers for
  short-circuit entry selection and plain conditional-branch render planning;
  avoid reopening them unless later work needs a new authoritative prepared
  per-terminator boundary rather than more wrapper glue.
- The compare-index split and branch compare-join handoff were already
  extracted before this packet; the remaining route should keep moving toward
  final Step 4.4 exhaustion, not back into earlier branch-helper reshuffles or
  Step 3 scalar-family cleanup.
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
Step 4.4 conditional-branch render selector split. The final `^backend_`
subset in `test_after.log` preserved the accepted `test_before.log` failure
set exactly, with no new failures:
`backend_codegen_route_x86_64_variadic_double_bytes_observe_semantic_bir`,
`backend_codegen_route_x86_64_variadic_pair_second_observe_semantic_bir`,
`backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir`,
and `backend_codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir`.
