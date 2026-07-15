# Current Packet

Status: Active
Source Idea Path: ideas/open/798_lir_stack_restore_lifetime_consumer_authority.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Verify the selected stack-restore authority

## Just Finished

- Plan Step 1: published the exact selected `LirStackRestoreOp` native
  admission, saved-checkpoint binding, and operation-local transition contract
  in `docs/lir_to_new_bir_remaining_coverage/798_stack_restore_native_authority_contract.md`.

## Suggested Next

- Execute Plan Step 2: implement and verify the selected stack-restore
  admission and checkpoint-transition contract with focused same-feature
  positive/negative coverage.

## Watchouts

- `local_object_authority.live` remains a checkpoint-binding validity fact;
  this route has no per-VLA allocation lifetime state and must not invent one.
  Do not absorb dynamic-VLA count work, VLA GEP, other local/lifetime rows,
  Raw-BIR/importer/734 receipt, or presentation-derived facts.

## Proof

- Documentation packet only: `git diff --check` plus structural inspection
  that the Step 1 note names selected admission, binding, transition, invalid
  cases, and current-versus-proposed facts; no build or test was required.
