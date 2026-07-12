# Current Packet

Status: Active
Source Idea Path: ideas/open/717_prepared_mir_join_source_identity_completion.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair typed join-source identity propagation

## Just Finished

- Step 2 localized the copied-block failure to the common preparation helper
  `attach_named_current_block_join_source_evidence` in
  `src/backend/prealloc/publication_plans.cpp`. The fixture builds
  `edge_publications` through the names/control-flow overload of
  `make_prepared_edge_publication_lookups`; that overload has no prepared BIR
  module, so its `PreparedEdgePublication` legitimately carries no
  `source_producer_block_label` or `source_producer_instruction_index`.
- The helper nevertheless treats those optional publication fields as the
  producer identity authority and returns `Incomplete` before selecting the
  supplied BIR evidence. For the copied block, the exact structural BIR fact is
  producer `(successor_label, incoming_name, instruction 3)`, and the supplied
  evidence agrees with it, so the row is expected to carry `Available` evidence
  with `value_name == incoming_name`. Instead it carries `Incomplete` evidence
  with the default/invalid value name. The prepared source/destination identity
  and homes remain the expected `incoming_id`, `&locations.value_homes[0]`, and
  `&locations.value_homes[1]`; the first incorrect fact is the missing evidence
  producer key, not either home.
- Route-agnostic repair rule: named join evidence must be selected against one
  exact producer identity established by the queried BIR block (function,
  producer block, produced value, instruction index), while publication-owned
  producer metadata may confirm that key when present but must not be required
  when the same exact BIR producer is already available. Continue to fail closed
  on absent, duplicate, or disagreeing producer identities; do not restore
  Route 5 selection or correlate by row position/name alone.

## Suggested Next

- Implement the bounded Step 2 common-helper repair in
  `src/backend/prealloc/publication_plans.cpp`: resolve the unique exact named
  producer key from `inputs.block`, pass that key into
  `attach_named_current_block_join_source_evidence`, use publication producer
  metadata only as an agreement check when it exists, and preserve the current
  typed `Missing`/`Incomplete`/`Ambiguous`/`Mismatched` fail-closed outcomes.

## Watchouts

- `block_has_matching_phi_publication` already proves the copied PHI publication
  agrees, but it records only `PublicationSemanticOrigin::BirPhi`; it does not
  publish the source producer block/index needed by evidence selection.
- Keep the repair in common preparation. Do not touch target materializers,
  restore Route 5 as authority, change move scheduling, or weaken the exact
  typed row mapping and aggregate fail-closed behavior.

## Proof

- No build or tests run, as delegated for this read-only localization packet.
  Evidence came from AST-backed definition/callee queries and focused reads of
  the fixture, edge-publication construction, and common join-source helpers.
  The existing `test_before.log` remains the supervisor-owned failure record;
  no test log was modified.
