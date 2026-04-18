# Execution State

Status: Active
Source Idea: ideas/open/58_bir_cfg_and_join_materialization_for_x86.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed a Step 3 Consume Prepared Control-Flow packet in
`src/backend/mir/x86/codegen/prepared_module_emit.cpp` by extracting entry
false-branch compare selection into helper-owned
`build_short_circuit_entry_compare_context()`. The cond-branch path now reuses
the same helper-owned entry compare context for prepared-branch lookup and
fallback compare rendering instead of re-growing that selection inline beside
the short-circuit orchestration path.

## Suggested Next

The next small Step 3 packet is to decide whether the remaining plain
true/false branch fallback render in the cond-branch path should join the same
short-circuit helper cluster so this emitter lane becomes entirely helper-
assembled rather than keeping one last inline branch-spelling fallback.

## Watchouts

- Do not activate umbrella idea 57 as executable work.
- Do not pull in idea 59 instruction-selection scope.
- Do not solve coverage gaps with x86 testcase-shaped matcher growth.
- Keep the compare-join lane aligned with the continuation lane: only the
  prepared predecessor recorded by the selected join-transfer edge is
  authoritative for continuation entry.
- `build_compare_join_continuation()` remains the Step 3 gate for the join-
  result zero-compare contract; keep `Eq`/`Ne` mapping and jump-target choice
  data-driven there instead of pushing them back into renderer assembly.
- `build_short_circuit_entry_routing_context()` now owns entry-branch contract
  validation, resolved true/false block lookup, and RHS-entry selection; keep
  that ownership in the helper cluster instead of re-growing those checks
  inside `try_render_short_circuit_plan()`.
- `build_short_circuit_entry_compare_context()` now owns prepared entry-branch
  lookup plus false-branch compare fallback selection; keep that ownership in
  the helper cluster instead of re-growing compare-shape selection beside the
  cond-branch emitter.
- `find_short_circuit_join_context()` now owns the prepared select-join shape
  checks plus continuation lookup; future contract edits should land there
  instead of re-growing those checks inside `try_render_short_circuit_plan()`.
- `build_short_circuit_plan()` now owns compare-true/compare-false target
  assembly; keep transfer-index validation and block-null checks there instead
  of re-growing them inside `try_render_short_circuit_plan()`.
- `try_render_short_circuit_plan()` now owns the short-circuit detect-plus-
  render handshake; future cleanup should keep pushing validation into the
  helper cluster instead of re-growing plan discovery in the cond-branch
  emitter.
- `build_short_circuit_render_context()` now owns rendered true/false lane
  omission,
  `render_short_circuit_target()` owns per-target block rendering plus
  continuation omission, `render_short_circuit_lanes()` now owns paired lane
  collection, `render_short_circuit_false_lane()` owns the false-label splice,
  and `assemble_short_circuit_rendered_plan()` owns the final string assembly;
  future cleanup should keep those responsibilities in helpers instead of re-
  growing inline target handling in `render_short_circuit_plan()`.
- `classify_short_circuit_join_incoming()` still assumes the prepared select
  join carries exactly one bool-like immediate lane and one named RHS lane; if
  that invariant changes, repair the shared contract instead of extending
  emitter-local pattern matching.
- Treat any future fix here as capability repair, not expectation weakening:
  the joined branch route is covered by `backend_x86_handoff_boundary`.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^backend_x86_handoff_boundary$' | tee test_after.log`.
The build and narrow proof passed for this Step 3 short-circuit entry compare
helper extraction packet; proof output is in `test_after.log`.
