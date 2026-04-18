# Execution State

Status: Active
Source Idea: ideas/open/58_bir_cfg_and_join_materialization_for_x86.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed a Step 3 Consume Prepared Control-Flow packet in
`src/backend/mir/x86/codegen/prepared_module_emit.cpp` by extracting short-
circuit render-lane omission choice into `build_render_context()`. The
renderer now reads as render-context lookup, per-target rendering, and final
label stitching instead of re-deriving rendered true/false lane ownership
inline.

## Suggested Next

The next small Step 3 packet is to move `render_short_circuit_target()` out of
`render_short_circuit_plan()` so the renderer becomes a pure control-flow
assembly step that delegates per-block continuation omission/rendering to one
reusable helper.

## Watchouts

- Do not activate umbrella idea 57 as executable work.
- Do not pull in idea 59 instruction-selection scope.
- Do not solve coverage gaps with x86 testcase-shaped matcher growth.
- Keep the compare-join lane aligned with the continuation lane: only the
  prepared predecessor recorded by the selected join-transfer edge is
  authoritative for continuation entry.
- `build_compare_join_continuation()` is the Step 3 gate for the join-result
  zero-compare contract; future cleanup here should keep `Eq`/`Ne` mapping and
  jump-target choice data-driven rather than pushing them back into the main
  matcher body.
- `build_render_context()` now owns the rendered true/false lane omission
  choice; if short-circuit continuation ownership changes, update that helper
  instead of re-growing inline lane-selection checks in
  `render_short_circuit_plan()`.
- `find_short_circuit_entry_blocks()` now owns the entry-condition and
  true/false successor validation for this path; if the prepared branch
  contract changes, update that helper instead of re-expanding the guard chain
  in `detect_short_circuit_plan_from_control_flow()`.
- `find_short_circuit_join_context()` now owns select-join lookup plus
  join-block and join-branch validation for this path; keep additional
  prepared join invariants there rather than folding them back into the main
  detector.
- `find_short_circuit_routing_context()` now owns the prepared incoming lane
  classification and compare-true/compare-false ownership split; if select
  joins gain new routing invariants, keep them there instead of rebuilding the
  split inline in the detector.
- `build_short_circuit_plan()` now owns compare-true/compare-false target
  construction; future short-circuit target validation should stay there
  instead of re-expanding plan assembly inside the detector.
- `classify_short_circuit_join_incoming()` assumes the prepared select join
  still carries exactly one bool-like immediate lane and one named RHS lane;
  if that invariant changes, fix the shared contract rather than re-growing
  emitter-local pattern matching.
- Do not treat `source_true_incoming_label` or `source_false_incoming_label` as
  the x86 continuation ownership contract; the authoritative predecessor still
  lives in `edge_transfers[source_*_transfer_index].predecessor_label`.
- Treat any future fix here as capability repair, not expectation weakening:
  the joined branch route is covered by `backend_x86_handoff_boundary`.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^backend_x86_handoff_boundary$' | tee test_after.log`.
The build and narrow proof passed for this Step 3 short-circuit render-lane
cleanup packet; `test_after.log` remains the canonical proof log path.
