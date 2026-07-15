# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 7.31
Current Step Title: Receive the selected VLA LirStackRestoreOp authority

## Just Finished

- Closed 794 completed the one selected `LirStackRestoreOp` producer handoff.
  Steps 1 through 7.30 remain accepted historical 734 receiver work and must
  not be repeated. The exact native authority contract is in
  `docs/lir_local_operation_authority/handoff_to_734.md`.

## Suggested Next

- Execute Step 7.31 only: receive closed 794's documented native stack-restore
  authority into the minimum typed Raw-BIR receiver path.

## Watchouts

- `local_object_authority.live` is checkpoint-binding validity, not a
  per-VLA allocation lifetime state. Consume only selected admission,
  `saved_ptr`, matching local authority, and the selected restore transition.
  Do not derive facts from presentation or absorb dynamic-VLA count/allocation,
  VLA GEP, other local/lifetime rows, or any later family.

## Proof

- Producer acceptance carried from closed 794: `cmake --build --preset default
  && ctest --test-dir build -j --output-on-failure -R
  '^frontend_lir_call_type_ref$'` passed 1/1. Before receiver implementation,
  run fresh build and focused receiver proof; supervisor selects broader proof.
