# Current Packet

Status: Active
Source Idea Path: ideas/open/794_lir_next_local_vla_authority_handoff.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Establish the candidate evidence boundary

## Just Finished

- Plan Step 1 evidence boundary completed: no remaining local/VLA row is
  selected. The record in
  `docs/lir_to_new_bir_remaining_coverage/794_local_vla_candidate_evidence_boundary.md`
  finds multiple partial local-object routes but no uniquely bounded native
  value/object/owner/type/liveness contract.

## Suggested Next

- Plan-owner decision: scope a separate producer/schema/verifier blocker for
  one explicit local/VLA admission (first missing fact: stack-restore lifetime
  transition, or dynamic-VLA typed count if that variant is chosen).

## Watchouts

- Closed 792 authorizes only VLA stack-save. Stack restore has saved-pointer
  object authority but no selected lifetime-consumer transition; dynamic VLA
  allocation has result/object authority but a non-native count. Do not select
  from presentation, `monostate`, or unresolved classifications, and do not
  change Raw-BIR.

## Proof

- `git diff --check`; structural inspection that the evidence note covers
  stack restore, dynamic VLA allocation, VLA GEP, nonselected local
  load/store/GEP, local temporaries, and lifetime consumers, with only the
  note and this canonical packet state changed. No build/test is required for
  this evidence-only packet.
