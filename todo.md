Status: Active
Source Idea Path: ideas/open/261_phase_f3_x86_route3_loadlocal_publication_fixture_support.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Fixture or Helper Implementation

# Current Packet

## Just Finished

Step 2 is blocked. A raw joined-branch fixture can add a true-lane
same-predecessor-block `LoadLocal` with a real local-slot `MemoryAddress`, and
the prepared edge-publication lookup path can naturally classify that
`incoming_value` as a `LoadLocal` source with available source-memory access.
However, the x86 compare-join route rejects that fixture before it reaches
`render_agreed_route3_load_local_statement_memory_operand(...)`: the
materialized compare-join return contract only classifies selected arms rooted
in immediates, params, same-module globals, or pointer-backed globals. It does
not classify a predecessor-block `LoadLocal` selected arm, so
`emit_prepared_module(...)` reports the compare-join source block has no
authoritative prepared join metadata/render contract for this shape.

No test changes were kept. Extending the route would require a production
compare-join helper/policy change to classify and render a selected `LoadLocal`
arm through the existing Route 3 statement-memory facade, which is broader than
this fixture/helper packet.

## Suggested Next

Return to plan-owner to split or rewrite the next packet around the missing
compare-join `LoadLocal` selected-arm support. The smallest plausible follow-up
is a bounded production helper extension that teaches the materialized
compare-join contract to classify a same-predecessor-block `LoadLocal` selected
arm and render it through the existing Route 3 statement-memory facade.

## Watchouts

- Do not revive the rejected synthetic `join_transfers` route from the
  addressed local-slot guard fixture.
- Do not count legacy no-publication fallback as positive Route 3/prepared
  agreement.
- The blocker is not missing publication construction: the natural publication
  path can see the `LoadLocal` and source-memory fact. The blocker is the
  compare-join return-context classifier/render whitelist before the x86 facade
  can consume that source.
- Adding this under Step 2 as a broad x86 route-policy rewrite would exceed the
  delegated fixture/helper ownership.

## Proof

Focused exploratory validation:
`cmake --build build-x86 --target backend_x86_handoff_boundary_test -j2`
passed before exploratory edits were reverted. The exploratory
`ctest --test-dir build-x86 --output-on-failure -R
'^backend_x86_handoff_boundary$'` failed with the unsupported compare-join
`LoadLocal` selected-arm route described above. No code/test diff remains.
