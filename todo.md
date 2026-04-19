# Execution State

Status: Active
Source Idea: ideas/open/58_bir_cfg_and_join_materialization_for_x86.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed a Step 3.3 Join-Transfer Carrier Coverage packet by extending the
join-route handoff proof in
`tests/backend/backend_x86_handoff_boundary_test.cpp` so
the adjacent same-module global selected-value route without fixed offsets,
without the selected-value chain, and without pointer-backed roots is now
covered under true-lane and false-lane passthrough topology drift, including
the `PreparedJoinTransferKind::EdgeStoreSlot` carrier shape, while the x86
consumer still follows the authoritative prepared join-transfer contract
instead of rediscovering ownership from join-block topology.

## Suggested Next

Stay in Step 3.3 and extend the same passthrough-topology carrier proof to the
adjacent same-module global selected-value chain route without fixed offsets
and without pointer-backed roots, keeping both the plain and `EdgeStoreSlot`
carrier variants covered before widening into new carrier families or Step 3.4
loop-cleanup work.

## Watchouts

- Keep this route in Step 3.3 join-transfer carrier coverage; do not slide
  back into Step 3.2 compare-join reproving, Step 3.4 loop cleanup, Step 4
  file organization, idea 57, idea 59, idea 60, idea 61, or the unrelated
  `^backend_` semantic-lowering failures.
- Carrier packets should keep proving that x86 consumes shared join-transfer
  ownership through prepared lookups even when predecessor topology drifts via
  passthrough blocks, rather than re-deriving ownership from local join-block
  shape.
- The new proof now locks in passthrough-topology drift for the non-fixed-
  offset non-pointer-backed same-module global selected-value carrier with and
  without `PreparedJoinTransferKind::EdgeStoreSlot`; the next packet should
  stay on the adjacent non-fixed-offset same-module global selected-value chain
  route before branching into broader carrier families.
- The broader `^backend_` checkpoint currently reproduces five known failures:
  `backend_prepare_phi_materialize`, `variadic_double_bytes`,
  `variadic_pair_second`, `local_direct_dynamic_member_array_store`, and
  `local_direct_dynamic_member_array_load`; do not widen this packet into
  those unrelated routes.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^backend_x86_handoff_boundary$' | tee test_after.log`.
The focused Step 3 handoff proof passed with the new non-fixed-offset
same-module global selected-value carrier coverage for true-lane and false-
lane passthrough topology drift, and preserved `test_after.log` at the repo
root.
