# Execution State

Status: Active
Source Idea: ideas/open/58_bir_cfg_and_join_materialization_for_x86.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed a Step 3 Consume Prepared Control-Flow packet in
`src/backend/mir/x86/codegen/prepared_module_emit.cpp` by extracting the
short-circuit entry-block validation plus RHS-routing choice into
`find_short_circuit_entry_routing_context()`. `try_render_short_circuit_plan()`
now consumes one entry/routing helper and one join-context helper instead of
rebuilding that prepared-data discovery inline.

## Suggested Next

The next small Step 3 packet is to trim `try_render_short_circuit_plan()`
around plan assembly by moving the compare-true/compare-false target build into
one helper so the planner becomes a straight-line sequence of prepared-context
lookups, plan assembly, and render assembly.

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
- `find_short_circuit_entry_routing_context()` now owns entry-branch contract
  validation, resolved true/false block lookup, and RHS-entry selection; keep
  future route checks there instead of re-growing those checks inside
  `try_render_short_circuit_plan()`.
- `find_short_circuit_join_context()` now owns the prepared select-join shape
  checks plus continuation lookup; future contract edits should land there
  instead of re-growing those checks inside `try_render_short_circuit_plan()`.
- `try_render_short_circuit_plan()` now owns the short-circuit detect-plus-
  render handshake; keep future cleanup inside that helper instead of
  re-growing plan discovery in the cond-branch emitter.
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
The build and narrow proof passed for this Step 3 entry-routing extraction
packet; `test_after.log` remains the canonical proof log path.
