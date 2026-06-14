Status: Active
Source Idea Path: ideas/open/261_phase_f3_x86_route3_loadlocal_publication_fixture_support.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Supported Publication Route Contract

# Current Packet

## Just Finished

Lifecycle switched from idea 258 after Step 4 exhausted the supported fixture
search. Idea 258 remains open and parked with its implementation facade in
place. This active runbook now owns the separate testability work needed to
create a supported x86 `LoadLocal` `source_memory_access` publication path into
`render_agreed_route3_load_local_statement_memory_operand(...)`.

## Suggested Next

Execute Step 1: inspect the existing x86 handoff publication builders and
fixture surfaces, then record the smallest supported route contract that can
reach the existing statement-memory agreement facade with a natural selected
`LoadLocal` source-memory publication.

## Watchouts

- Do not revive the rejected synthetic `join_transfers` route from the
  addressed local-slot guard fixture.
- Do not count legacy no-publication fallback as positive Route 3/prepared
  agreement.
- Keep implementation work limited to supported fixture/testability support;
  broad x86 target-policy rewrites belong outside this idea.

## Proof

Lifecycle-only switch. Existing executor proof for the blocked idea 258 Step 4
is in `test_after.log`; no code validation was required for this planning
change.
