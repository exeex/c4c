# Execution State

Status: Active
Source Idea: ideas/open/58_bir_cfg_and_join_materialization_for_x86.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed a Step 3 Consume Prepared Control-Flow packet in
`src/backend/mir/x86/codegen/prepared_module_emit.cpp` by removing the copied
`classified_incoming` field from `ShortCircuitEntryRoutingContext` and routing
`build_short_circuit_plan()` through the lane classification already owned by
`ShortCircuitJoinContext`. Short-circuit entry routing now carries only direct
branch targets and RHS-entry selection, while join-lane ownership stays on the
prepared join helper contract.

## Suggested Next

The next small Step 3 packet is to inspect whether the short-circuit direct-
branch fallback and compare-driven render paths can share one helper-owned
branch target contract, so compare-and-branch assembly stops carrying parallel
`true_block` / `false_block` plumbing beside `resolve_direct_branch_targets()`.

## Watchouts

- Do not activate umbrella idea 57 as executable work.
- Do not pull in idea 59 instruction-selection scope.
- Do not solve coverage gaps with x86 testcase-shaped matcher growth.
- Keep the compare-join lane aligned with the continuation lane: only the
  prepared predecessor recorded by the selected join-transfer edge is
  authoritative for continuation entry.
- `find_short_circuit_join_context()` now owns only prepared select-join
  discovery and transfer-shape validation; keep join-block lookup, prepared
  join-branch lookup, and continuation-plan assembly in
  `build_short_circuit_join_context_from_transfer()` instead of re-growing
  those checks beside join discovery.
- `build_short_circuit_join_context_from_transfer()` now also owns short-
  circuit lane classification; keep join-input ownership validation there
  instead of re-growing `classify_short_circuit_join_incoming()` calls in entry
  routing or render paths.
- `ShortCircuitEntryRoutingContext` now carries only direct branch targets and
  RHS-entry routing; do not re-copy join-lane classification there when
  `ShortCircuitJoinContext` already owns the prepared classification contract.
- `build_compare_join_continuation()` remains the Step 3 gate for the join-
  result zero-compare contract; keep `Eq`/`Ne` mapping and jump-target choice
  data-driven there instead of pushing them back into renderer assembly.
- `resolve_direct_branch_targets()` now owns direct true/false block lookup for
  plain cond-branch entry, short-circuit entry routing, and compare-join
  continuation successor selection; keep block-null and self-target rejection
  there instead of re-growing local checks in emitter paths.
- `render_compare_driven_branch_plan()` still owns compare-driven lane
  rendering once a plan exists; future cleanup should route compare-and-branch
  assembly through that helper instead of re-growing inline false-label
  strings.
- `classify_short_circuit_join_incoming()` still assumes the prepared select
  join carries exactly one bool-like immediate lane and one named RHS lane; if
  that invariant changes, repair the shared contract instead of extending
  emitter-local pattern matching.
- Treat any future fix here as capability repair, not expectation weakening:
  the joined branch route is covered by `backend_x86_handoff_boundary`.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^backend_x86_handoff_boundary$' | tee test_after.log`.
The build and narrow proof passed for this Step 3 routing-context ownership
cleanup packet; proof output is in `test_after.log`.
