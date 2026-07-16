Status: Active
Source Idea Path: ideas/open/866_lir_remaining_authority_owner_triage.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Generate ordered follow-up ideas and close the umbrella

# Current Packet

## Just Finished

Completed Step 2 by creating
`docs/lir_remaining_authority_owner_triage/classification.md`. The
classification separates documentation/research, LIR producer/schema, LIR
verifier, collector/import-preparation, Raw-BIR receiver, and terminal policy
ownership; rejects generic residual sweeps as current successors; and
reconciles stale overlapping open ideas against post-Step-7.51 734 state.

## Suggested Next

Execute Step 3 by generating or repairing ordered follow-up ideas under
`ideas/open/` from the classification, then ask plan-owner to close or
otherwise disposition the umbrella.

## Watchouts

Do not implement code or tests in this umbrella. Keep each generated follow-up
single-owner and single-layer unless the source explicitly requires a bounded
handoff/return pairing. Do not route 734 Raw-BIR receiver work before a typed
LIR handoff exists, and do not reopen accepted `LirAbsOp`, direct-call
argument 0/1, body-parameter, local-object/VLA, or call-result rows.

## Proof

Docs-only Step 2 proof: `git diff --check`. No `test_after.log` was written
because the delegated proof command is documentation-only and produces no
test log.
