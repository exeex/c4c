Status: Active
Source Idea Path: ideas/open/423_rv64_runtime_mismatch_triage.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Create Follow-Up Ideas Or Park Unclear Rows

# Current Packet

## Just Finished

Completed `plan.md` Step 5 by creating durable implementation follow-up ideas
for the two high-confidence ordinary-C runtime mismatch owner groups:
`ideas/open/443_rv64_prepared_value_operand_materialization.md`, starting from
`src/pr81503.c`, and
`ideas/open/444_frame_slot_call_argument_publication.md`, starting from
`src/20080506-2.c`. Both ideas include reviewer reject signals against
testcase-shaped runtime fixes, expectation or unsupported-marker changes,
pass/fail-accounting changes, and source-name shortcuts.

Parked unresolved runtime rows instead of assigning speculative owners:
31 abort rows outside `src/pr38533.c`, 6 segfault rows outside
`src/20080506-2.c`, and stale/nondeterministic `src/20000314-2.c`. The inline
asm tied-output/result materialization representative `src/pr38533.c` remains
deferred as lower ordinary-C priority unless a later supervisor packet
explicitly activates that route.

## Suggested Next

Execute Step 6 lifecycle close check for active idea 423. Verify the source
idea acceptance criteria against the Step 1-5 handoff artifacts and decide
whether to close the triage idea or replace the runbook with narrower
remaining triage work.

## Watchouts

- This remains a triage runbook; implementation belongs to the new follow-up
  ideas, not active idea 423.
- Do not weaken runtime comparison, expected output, unsupported markers, or
  pass/fail accounting during close.
- Do not assign the parked 31 abort rows or 6 segfault rows to the new ideas
  based only on runtime exit mode.
- `src/20000314-2.c` should remain stale/nondeterministic for this triage until
  a later scan reproduces the original abort again.
- Inline asm tied-output materialization is clear for `src/pr38533.c`, but it
  remains parked/deferred unless the supervisor chooses to create that
  lower ordinary-C priority idea later.

## Proof

Passed:
`git diff --check -- todo.md`.
`git diff --no-index --check /dev/null ideas/open/443_rv64_prepared_value_operand_materialization.md; test $? -le 1`.
`git diff --no-index --check /dev/null ideas/open/444_frame_slot_call_argument_publication.md; test $? -le 1`.
