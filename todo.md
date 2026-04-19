# Execution State

Status: Active
Source Idea: ideas/open/58_bir_cfg_and_join_materialization_for_x86.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed a Step 3.3 Join-Transfer Carrier Coverage packet by re-proving the
focused join-route handoff coverage in
`tests/backend/backend_x86_handoff_boundary_test.cpp` for the adjacent same-
module global selected-value chain route without fixed offsets and without
pointer-backed roots under true-lane and false-lane passthrough topology
drift, including the `PreparedJoinTransferKind::EdgeStoreSlot` carrier shape,
while the x86 consumer still follows the authoritative prepared join-transfer
contract instead of rediscovering ownership from join-block topology. A
reviewer check in `review/step3_3_route_drift_review.md` confirmed that the
focused Step 3.3 carrier matrix is now saturated enough that more generic
carrier hunting would be proof churn rather than fresh capability progress.

## Suggested Next

Move to Step 3.4 and target one concrete residual consumer path in
`src/backend/mir/x86/codegen/prepared_module_emit.cpp`, starting with loop-
carried join-transfer handling that still depends on emitter-local recovery
instead of authoritative prepared transfer data, then prove that slice with
focused backend/x86 coverage.

## Watchouts

- Keep this route in Step 3.4 residual consumer cleanup; do not slide back
  into Step 3.2 compare-join reproving, generic Step 3.3 carrier hunting,
  Step 4 file organization, idea 57, idea 59, idea 60, idea 61, or the
  unrelated `^backend_` semantic-lowering failures.
- Carrier packets should keep proving that x86 consumes shared join-transfer
  ownership through prepared lookups even when predecessor topology drifts via
  passthrough blocks, rather than re-deriving ownership from local join-block
  shape.
- Treat generic Step 3.3 carrier hunting as exhausted unless a specifically
  identified uncovered join-transfer consumer path appears; do not spend more
  packets searching for another adjacent passthrough testcase variant.
- The next packet should stay on Step 3.4 residual consumer cleanup, with
  emphasis on loop-carry handling in `prepared_module_emit.cpp`, rather than
  sliding back into Step 3.2 compare-join return-context reproving or Step 4
  file organization.
- The broader `^backend_` checkpoint currently reproduces five known failures:
  `backend_prepare_phi_materialize`, `variadic_double_bytes`,
  `variadic_pair_second`, `local_direct_dynamic_member_array_store`, and
  `local_direct_dynamic_member_array_load`; do not widen this packet into
  those unrelated routes.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^backend_x86_handoff_boundary$' | tee test_after.log`.
The focused Step 3 handoff proof passed with the new non-fixed-offset
same-module global selected-value chain coverage for true-lane and false-lane
passthrough topology drift, and preserved `test_after.log` at the repo root.
