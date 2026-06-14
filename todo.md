Status: Active
Source Idea Path: ideas/open/256_phase_f3_prepared_liveness_authority_blocker_map.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Liveness Consumer Inventory

# Current Packet

## Just Finished

Lifecycle activation created the runbook and canonical execution-state
skeleton for Step 1 - Liveness Consumer Inventory.

## Suggested Next

Execute Step 1 by inventorying every `PreparedBirModule::liveness` producer,
consumer, derived helper, and test/reference row across prealloc, regalloc,
helper planning, x86, riscv, prepared printer, and status/debug surfaces.

## Watchouts

- This is analysis-only; do not edit implementation code or expectations under
  this active plan.
- Do not treat the completed route/invariants/completed_phases/notes metadata
  gate as evidence that `liveness` is demotion-ready.
- Mark allocation, storage, helper-planning, target-register,
  target-storage, move-scheduling, and output-policy consumers as blockers
  unless a later source idea proves a narrower semantic handoff.

## Proof

Lifecycle activation only. Required check: `git diff --check -- plan.md todo.md`.
