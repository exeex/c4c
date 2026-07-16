Status: Active
Source Idea Path: ideas/open/830_lir_direct_call_structured_argument_identity_prerequisite.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prove Direct-Call Argument-1 Completion And Return 829

# Current Packet

## Just Finished

Completed Step 3. Closed 831 accepted the repaired comparable full-suite
baseline gate at 3038/3038, so no 830 code change is needed for the post-Step-2
baseline acceptance.

## Suggested Next

Run Step 4 focused completion proof for the bounded direct-call argument-1
identity/type relation. Use a fresh build plus `frontend_lir_call_type_ref`,
then close 830 if the proof remains green and the completion record can return
829 to Step 2.

## Watchouts

- Do not edit code for Step 3.
- Do not publish 829 body-parameter authority inside 830.
- Do not broaden to generic call arguments or other indices.

## Proof

Closed 831 accepted full-suite comparable baseline: 3038/3038 passed at commit
`4be820759` in `test_baseline.log`.
