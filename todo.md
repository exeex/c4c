Status: Active
Source Idea Path: ideas/open/23_x86_prepared_edge_publication_remaining_home_coverage.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Handoff RISC-V Readiness Decision

# Current Packet

## Just Finished

Completed Step 5 handoff for idea 23. Based on the committed x86
implementation and `review/idea23_x86_immediate_edge_publication_review.md`,
the shared prepared edge-publication path is now ready for a separate RISC-V
consumer idea scoped to register-destination edge moves.

Current x86 coverage through shared `edge_publications` includes:

- `StackSlot -> Register` edge-publication moves.
- `Register -> Register` edge-publication moves.
- `RematerializableImmediate -> Register` edge-publication moves when the
  source home publishes `immediate_i32`.

Concrete decision: a RISC-V consumer idea is justified now for the
register-destination subset because x86 has demonstrated multiple independent
source-home families through the shared publication lookup instead of a
testcase-shaped or x86-only edge scan. Another x86 coverage slice does not need
to come first for that scoped RISC-V consumer work.

This is not a full-home parity claim. `PointerBasePlusOffset -> Register` and
source-to-`StackSlot` destination moves remain unsupported and fail-closed on
x86.

## Suggested Next

Supervisor should route lifecycle closure for idea 23 and open or activate a
separate RISC-V prepared edge-publication consumer idea scoped to register
destinations.

## Watchouts

- Keep the RISC-V follow-up scoped to the proven register-destination handoff.
- Do not claim readiness for pointer-base sources or stack-slot destinations.
- If RISC-V needs stack-slot destination publication before useful consumer
  work can land, create a separate x86 destination-home coverage slice first.

## Proof

Docs/lifecycle-only handoff note. No build or tests were required, and no
`test_after.log` was created for this packet.
