# Execution State

Status: Active
Source Idea: ideas/open/58_bir_cfg_and_join_materialization_for_x86.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed a Step 3.2 Compare-Join Consumer Migration packet by extending the
compare-join handoff proof in
`tests/backend/backend_x86_handoff_boundary_test.cpp` so
`render_materialized_compare_join_if_supported()` is now proven against the
adjacent fixed-offset same-module global selected-value-chain route without
pointer-backed roots, including the `PreparedJoinTransferKind::EdgeStoreSlot`
carrier shape, while the entry compare carrier has been rewritten into a
non-compare instruction and the x86 consumer still follows the established
compare-join render contract.

## Suggested Next

Stay in Step 3.2 on the same compare-join route and add the next focused
non-compare-entry-carrier handoff proof for the adjacent fixed-offset
same-module global selected-value route without the selected-value chain and
without pointer-backed roots; after that packet is either complete or blocked,
re-evaluate whether remaining work belongs in Step 3.2 or Step 3.3 instead of
leaving it under an undifferentiated Step 3 label.

## Watchouts

- Keep this route in Step 3.2 compare-join consumer work unless the packet
  clearly shifts into Step 3.3 join-transfer carrier coverage; do not widen
  into Step 4 file organization, idea 57, idea 59, idea 60, idea 61, or the
  unrelated `^backend_` semantic-lowering failures.
- Compare-join packets should keep proving that x86 consumes the shared
  resolved render contract for branch labels, join-transfer ownership, and
  selected-value return arms instead of re-deriving meaning from entry compare
  carriers or join-block topology.
- The new proof now locks in non-compare entry-carrier drift for same-module
  global selected-value-chain compare-join routes with and without
  pointer-backed roots, plus the fixed-offset non-pointer-backed route and the
  current pointer-backed selected-value routes with and without the fixed-offset
  variant, including the `EdgeStoreSlot` carrier shape; the adjacent fixed-offset
  non-pointer-backed selected-value route without the chain remains a separate
  follow-up packet.
- The broader `^backend_` checkpoint currently reproduces five known failures:
  `backend_prepare_phi_materialize`, `variadic_double_bytes`,
  `variadic_pair_second`, `local_direct_dynamic_member_array_store`, and
  `local_direct_dynamic_member_array_load`; do not widen this packet into
  those unrelated routes.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^backend_x86_handoff_boundary$' | tee test_after.log`.
The focused Step 3 handoff proof passed with the new compare-join fixed-offset
same-module global selected-value-chain route coverage for non-compare
entry-carrier drift and preserved `test_after.log` at the repo root.
