# Execution State

Status: Active
Source Idea: ideas/open/58_bir_cfg_and_join_materialization_for_x86.md
Source Plan: plan.md
Current Step: Step 3.4 Loop-Carry And Residual Consumer Cleanup
Last Plan Review Counter: 1

# Current Packet

## Just Finished

Completed a Step 3.4 Loop-Carry And Residual Consumer Cleanup packet by
tightening `render_local_slot_guard_chain_if_supported()` so the short-circuit
entry path now throws once a function carries authoritative prepared
branch-owned join ownership but the entry block can no longer build a valid
prepared short-circuit render plan, instead of slipping into the plain
conditional lane, and by adding regressions for both the select-carried and
`EdgeStoreSlot` variants that reset the authoritative entry prepared branch
contract.

## Suggested Next

Stay in Step 3.4 and inspect the remaining residual helper order after the
sealed rhs-compare and entry-branch fallbacks, especially any remaining
authoritative branch-owned join routes outside
`render_local_slot_guard_chain_if_supported()` that can still degrade from a
prepared-control-flow rejection into a generic guard-family acceptance.

## Watchouts

- Keep this route in Step 3.4 residual consumer cleanup; do not widen into
  Step 3.2 compare-join reproving, Step 3.3 carrier hunting, Step 4 file
  organization, idea 57, idea 59, idea 60, idea 61, or the unrelated
  `^backend_` semantic-lowering failures.
- The local-slot short-circuit rhs continuation must now reject once a prepared
  rhs branch condition exists but no longer yields a valid prepared compare
  context; do not reopen the raw trailing-compare fallback behind that
  authoritative contract.
- The local-slot short-circuit entry branch must now reject once authoritative
  branch-owned join ownership exists but the entry prepared branch contract has
  drifted; do not let that route fall through into the generic plain-condition
  lane.
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
The focused Step 3.4 handoff proof passed after rejecting drifted authoritative
entry prepared branch metadata in both select-carried and `EdgeStoreSlot`
local short-circuit routes instead of falling through to the plain conditional
lane, leaving `test_after.log` at the repo root.
