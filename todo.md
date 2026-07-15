# Current Packet

Status: Active
Source Idea Path: ideas/open/794_lir_next_local_vla_authority_handoff.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Publish and verify the selected producer authority

## Just Finished

- Closed 798 completed its selected `LirStackRestoreOp` native-authority
  handoff. 794 Step 1 evidence remains accepted in `1cbad00d6`; do not redo
  the candidate inspection.

## Suggested Next

- Execute 794 Step 2 using only closed 798's exact selected stack-restore
  authority and focused producer proof boundary.

## Watchouts

- The handoff is to 794, not 734. `local_object_authority.live` remains a
  checkpoint-binding validity fact; the transition does not create per-VLA
  allocation lifetime state. Do not absorb dynamic-VLA count work, VLA GEP,
  other local/lifetime rows, Raw-BIR/importer/734 receipt, or
  presentation-derived facts.

## Proof

- Closed 798 acceptance: fresh build, focused
  `^frontend_lir_call_type_ref$` proof passing 1/1, non-decreasing 1/1 guard,
  and broader frontend smoke. The implementation proof is `test_after.log`;
  execute fresh Step 2 proof before accepting new producer work.
