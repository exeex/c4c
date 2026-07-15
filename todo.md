# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 7.26
Current Step Title: Receive selected hoisted alloca authority

## Just Finished

- Resumed after accepted 752 closure. Historical Steps 1 through 7.25,
  including accepted parallel-edge PHI receipt, remain complete and must not
  be repeated.

## Suggested Next

- Execute only Step 7.26: receive the selected hoisted `LirAllocaOp` using its
  `result` and `local_object_authority` fields `pointer_definition`, `object`,
  `owner`, `pointer_type`, `pointee_type`, and `live`.

## Watchouts

- Reject missing, invalid, foreign, pointer/object/type-mismatched, dead, or
  disagreeing authority transactionally. Do not derive identity from local
  names or `%t`; do not absorb local load/store/GEP, VLA lifetime, memory/va,
  aggregate/vector, body parameters, PHI/CFG, or later families.

## Proof

- Step 7.26 requires a fresh build and narrow receiver proof; the supervisor
  selects any broader acceptance validation. Producer handoff evidence is
  `ca26a8242`, `b200ac033`, and `0e8093025`, with accepted focused and
  `^backend_` 5/5 proof plus fresh full CTest 3037/3037.
