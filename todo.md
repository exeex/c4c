# Execution State

Status: Active
Source Idea: ideas/open/58_bir_cfg_and_join_materialization_for_x86.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed a Step 3.4 Loop-Carry And Residual Consumer Cleanup packet by
tightening `render_local_i32_arithmetic_guard_if_supported()` to consume the
authoritative prepared branch contract when one exists, and by adding a
regression that drifts prepared guard labels to prove x86 now rejects that
route instead of falling back to the legacy local guard matcher.

## Suggested Next

Stay in Step 3.4 and inspect the remaining residual helper order after the
prepared loop and local-guard consumers, especially `render_local_slot_guard_chain_if_supported()`,
for any other branch/join routes where prepared metadata can drift out of
contract but a later raw-shape matcher still accepts the function.

## Watchouts

- Keep this route in Step 3.4 residual consumer cleanup; do not widen into
  Step 3.2 compare-join reproving, Step 3.3 carrier hunting, Step 4 file
  organization, idea 57, idea 59, idea 60, idea 61, or the unrelated
  `^backend_` semantic-lowering failures.
- The local i32 guard fallback now trusts prepared branch labels and compare
  semantics when present; any remaining residual helper should follow the same
  contract boundary rather than re-reading raw compare carriers.
- The local countdown fallback must stay unavailable once a function carries
  authoritative `PreparedJoinTransferKind::LoopCarry` ownership; do not reopen
  that bypass while chasing other residual helpers.
- The broader `^backend_` checkpoint currently reproduces five known failures:
  `backend_prepare_phi_materialize`, `variadic_double_bytes`,
  `variadic_pair_second`, `local_direct_dynamic_member_array_store`, and
  `local_direct_dynamic_member_array_load`; do not widen this packet into
  those unrelated routes.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^backend_x86_handoff_boundary$' | tee test_after.log`.
The focused Step 3.4 handoff proof passed after rejecting drifted prepared
local-guard labels instead of falling through to the local guard matcher,
leaving `test_after.log` at the repo root.
