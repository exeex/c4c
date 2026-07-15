# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 7.31
Current Step Title: Receive the selected VLA LirStackRestoreOp authority

## Just Finished

- Step 7.31 received exactly the selected VLA `LirStackRestoreOp` as typed
  Raw-BIR stack-restore authority, consuming the matching selected stack-save
  checkpoint with native saved-pointer, object/owner, pointer/pointee, live,
  and transition facts. Nearby receiver coverage proves transactional
  acceptance and malformed/unselected rejection.

## Suggested Next

- Supervisor to inspect this coherent Step 7.31 slice, compare the canonical
  regression logs, and decide the next lifecycle action; do not extend the
  local/VLA receiver surface from this packet.

## Watchouts

- `local_object_authority.live` remains checkpoint-binding validity, not a
  per-VLA allocation lifetime state. The restore receiver remains selected-only
  and fail closed for dynamic-VLA allocation/count, VLA GEP, other local or
  lifetime rows, and presentation-derived recovery.

## Proof

- `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^backend_' > test_after.log` (running for this
  packet) passed 5/5; `test_after.log` is the required proof log.
