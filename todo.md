# Current Packet

Status: Active
Source Idea Path: ideas/open/720_block_entry_publication_proof_evidence_multiplicity.md
Source Plan Path: plan.md
Current Step ID: 6.1
Current Step Title: Restore conservative production compatibility

## Just Finished

- Plan Step 6 final acceptance was rejected by
  `review/idea720_step6_final_acceptance_review.md`; the route remains active
  and has been split into bounded repair and proof substeps 6.1 through 6.5.

## Suggested Next

- Execute Plan Step 6.1: restore conservative production compatibility without
  name-based availability recovery or duplicate fail-closed downgrade.

## Watchouts

- Do not hand back to idea 718 until Steps 6.1 through 6.5 and the final audit
  are complete.
- The legacy pointer overload is a production boundary: it must not turn a
  display-name/type scan into a synthetic available claim or lose the former
  duplicate fail-closed behavior.
- Keep Route4 authoritative for the complete modeled coordinate, including
  `instruction_owner_label_id`.

## Proof

- Pending supervisor-delegated build and focused proof for Plan Step 6.1.
