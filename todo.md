# Execution State

Status: Active
Source Idea Path: ideas/open/59_generic_scalar_instruction_selection_for_x86.md
Source Plan Path: plan.md
Current Step ID: 4.4
Current Step Title: Finish Remaining Covered Branch And Residual Call Families
Plan Review Counter: 3 / 10
# Current Packet

## Just Finished

Step 4.4 split the bounded same-module helper call lane in
`prepared_local_slot_render.cpp` into a selector-oriented render contract:
`select_prepared_bounded_same_module_helper_call_render_if_supported` now
decides whether the covered helper call is admitted and pre-renders its
argument moves plus callee spelling before
`render_prepared_bounded_same_module_helper_call_if_supported` applies the
post-call state update.

## Suggested Next

Treat the same-module helper call lane as structurally exhausted again for
Step 4.4 unless a later audit finds a genuinely new prepared selector contract
beyond the new helper-call render selection and post-call state split;
otherwise move the next packet to the remaining residual Step 4.4 bounded
multi-defined call or branch route instead of reopening this helper lane.

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
- The inline terminator compare-index split is now also extracted; avoid
  reopening the main block renderer unless a later packet needs a new
  selector boundary rather than another local helper shuffle.
- The bounded same-module helper call lane now has an explicit selector/result
  split for argument moves and callee spelling; avoid reopening it unless a
  later residual call route needs another authoritative prepared per-call
  contract beyond that selector boundary.
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
Step 4.4 same-module helper-call render selector split. The final `^backend_`
subset in `test_after.log` preserved the accepted `test_before.log` failure
set exactly, with no new failures:
`backend_codegen_route_x86_64_variadic_double_bytes_observe_semantic_bir`,
`backend_codegen_route_x86_64_variadic_pair_second_observe_semantic_bir`,
`backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir`,
and `backend_codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir`.
