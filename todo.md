# Execution State

Status: Active
Source Idea: ideas/open/58_bir_cfg_and_join_materialization_for_x86.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed a Step 3 Consume Prepared Control-Flow In X86 proof packet by adding
focused x86 handoff coverage in
`tests/backend/backend_x86_handoff_boundary_test.cpp` that proves both the
plain and `PreparedJoinTransferKind::EdgeStoreSlot` materialized compare-join
routes still emit the authoritative prepared entry-branch contract even when
the entry compare carrier is rewritten to unrelated non-compare state; the x86
consumer path already matched the shared prepared-control-flow contract, so no
emitter code change was required for this slice.

## Suggested Next

Stay in Step 3 and extend the same prepared-contract proof to another
materialized compare-join consumer seam, especially a label-drift or rendered
same-module-global path that could still fall back to raw CFG labels instead
of the authoritative prepared branch plan.

## Watchouts

- Keep this route in Step 3 consumer work; do not widen into Step 4 file
  organization, idea 57, idea 59, idea 60, idea 61, or the unrelated
  `^backend_` semantic-lowering failures.
- Materialized compare-join consumers should keep preferring prepared entry
  branch metadata over mutated source compare carriers whenever both exist.
- This packet confirmed the x86 compare-join consumer was already aligned with
  that shared branch-contract ownership; keep future work focused on missing
  consumer proof or real capability gaps, not redundant emitter churn.
- The broader `^backend_` checkpoint currently reproduces five known failures:
  `backend_prepare_phi_materialize`, `variadic_double_bytes`,
  `variadic_pair_second`, `local_direct_dynamic_member_array_store`, and
  `local_direct_dynamic_member_array_load`; do not widen this packet into
  those unrelated routes.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^backend_x86_handoff_boundary$' | tee test_after.log`.
The focused Step 3 handoff proof passed with the new materialized compare-join
entry-carrier coverage and preserved `test_after.log` at the repo root.
