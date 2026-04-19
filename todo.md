# Execution State

Status: Active
Source Idea: ideas/open/58_bir_cfg_and_join_materialization_for_x86.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed a Step 3 Consume Prepared Control-Flow packet in
`tests/backend/backend_x86_handoff_boundary_test.cpp` and `todo.md` by
extending the prepared compare-join return-context ownership checks for the
fixed-offset pointer-backed same-module global selected-value chain family so
both the plain prepared helper path and the paired
`PreparedJoinTransferKind::EdgeStoreSlot` carrier keep publishing the same
return-context contract when one extra empty authoritative bridge sits on
either the true lane or the false lane before the join.

## Suggested Next

Before any more Step 3 compare-join passthrough-family execution, treat the
accumulated shared-helper and x86-consumer churn as needing broader proof than
`^backend_x86_handoff_boundary$`. After that broader proof checkpoint, the next
accepted Step 3 packet should move to a materially different prepared
control-flow consumer seam instead of another adjacent one-extra-bridge
selected-value variant.

## Watchouts

- Keep this family in Step 3 semantic consumer helpers; do not widen into Step
  4 file organization, idea 57, idea 59, idea 60, idea 61, or countdown-loop
  route changes.
- A route review judged the route `drifting`, not because Step 3 failed, but
  because packet selection and proof discipline narrowed too far around one
  compare-join passthrough family. Do not queue another adjacent
  passthrough-coverage packet until the broader proof gap is addressed.
- Do not solve remaining compare-join gaps with x86-side CFG scans,
  testcase-shaped matcher growth, or broad multi-block rediscovery. This
  family should only allow one extra empty passthrough after an already-
  authoritative compare lane when the prepared branch labels and join-transfer
  ownership already identify the real source edges, including the
  fixed-offset pointer-backed selected-value chain return-context variant that
  now has both true-lane and false-lane passthrough proof for the plain helper
  path and the paired EdgeStoreSlot carrier without any backend change.
- Keep follow-on work focused on places where prepared branch labels and join
  ownership are already authoritative; do not reintroduce source-label
  equality checks, local join bundle reconstruction, or emitter-local semantic
  recovery.
- Treat the optional-empty-passthrough recognition in shared helpers as route
  debt, not as a license to keep cloning more same-shape variants. Follow-on
  work should either prove the broader route or strengthen prepared ownership
  more generally.
- When a test helper appends passthrough blocks to `function.blocks`, reacquire
  any cached block pointers before calling prepared-helper classifiers; this
  packet needed that harness-only fix to keep the proof scoped to the intended
  ownership contract.
- `test_before.log` remains the last narrow baseline for
  `^backend_x86_handoff_boundary$`, but the review concluded that this is no
  longer sufficient acceptance evidence for the current Step 3 blast radius.
  The next supervisor-routed proof must broaden beyond that single named
  subset before more same-family execution is accepted.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^backend_x86_handoff_boundary$' | tee test_after.log`.
The focused proof refreshes `test_after.log` with the
`backend_x86_handoff_boundary` subset for the new fixed-offset pointer-backed
same-module global selected-value chain true-lane and false-lane
return-context passthrough ownership coverage, the paired EdgeStoreSlot
carrier coverage, and the existing prepared branch/join ownership families
that continue proving the same handoff contracts. The proof passed.

Route review note: this narrow proof is no longer sufficient for continued
acceptance of the current Step 3 route. No broader replacement proof has been
run yet in this repair.
