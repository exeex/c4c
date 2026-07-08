Status: Active
Source Idea Path: ideas/open/589_direct_edge_publication_move_freshness_ownership.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Direct Edge-Publication Move Consumers

# Current Packet

## Just Finished

Step 1 audit completed for direct edge-publication move consumers and helper
routes. Audited consumers:

- `make_prepared_edge_publication_lookups` builds the direct edge-publication
  row from join edge transfers, destination homes, source homes, source
  producer facts, and `find_edge_publication_move`; it is a row publisher, not
  the migration consumer, and currently does not prove source freshness.
- `prepare_edge_copy_source_facts`,
  `find_unique_indexed_block_entry_parallel_copy_edge_publication`, and
  `prepare_block_entry_parallel_copy_edge_source_facts` consume indexed
  edge-publication rows plus block-entry out-of-SSA move facts. They currently
  validate publication/move/source-home identity and fail closed through
  `PreparedEdgeCopySourceFactsStatus`, but do not query shared freshness.
- `prepare_current_block_join_parallel_copy_source_facts` consumes
  `prepare_block_entry_parallel_copy_edge_source_facts` for current-block join
  source facts, then enriches destination/source homes and Route5 agreement.
- `prepare_aggregate_stack_source_authority` consumes publication rows,
  stack source homes, destination register placement, and the matching
  out-of-SSA move; it stops at `MissingAggregateCopyAuthority`.
- `prepare_same_width_i32_stack_source_publication` consumes publication rows,
  stack source homes, destination register placement, and the matching move
  for same-width I32 stack-source publication.
- `classify_prepared_object_move_bundle_consumer` consumes move bundles through
  `PreparedValueFreshnessUseKind::MoveBundleSource`; this family is already
  protected by the shared freshness query.
- `plan_prepared_store_source_publication` /
  `publish_store_source_producer_freshness_authority` and
  `plan_prepared_dependency_operand_authority` consume producer-publication
  operands through `PreparedValueFreshnessUseKind::ProducerPublicationOperand`;
  these families are already protected by idea 587/588 freshness checks.
- `plan_prepared_select_edge_source_producer_placement`,
  `collect_prepared_select_edge_source_producer_placements`,
  `populate_stack_destination_register_fan_in_move_authority`, and
  select-carrier alias authority collection consume edge-publication rows or
  nearby move bundles for select suppression, destination fan-in legality, or
  alias evidence; none of those destination/alias facts is source freshness.

Selected representative migration route:
`prepare_block_entry_parallel_copy_edge_source_facts` is the smallest direct
edge-publication move route to migrate. It has the clearest prepared
publication row, matching `PreparedMoveResolution`, source and destination
value identity, source home, predecessor/successor edge identity, and existing
fail-closed status surface for missing publication, ambiguous publication,
edge mismatch, move mismatch, unsupported move, and missing source facts.
The next packet can add/query a narrow direct edge-publication source freshness
authority there without redesigning typed stack-source, aggregate stack-source,
select-carrier aliasing, or target backend emission.

Unselected family classifications:

- Protected: prepared object move-bundle source consumers are protected by
  `MoveBundleSource` freshness; store-source and dependency operand
  publication consumers are protected by `ProducerPublicationOperand`
  freshness.
- Deferred for scope: `make_prepared_edge_publication_lookups` remains the row
  publisher/helper, and `prepare_current_block_join_parallel_copy_source_facts`
  should inherit the selected route after the representative facts helper is
  migrated.
- Blocked on missing producer/publication facts: typed stack-source and
  aggregate stack-source publications need concrete stack-source freshness
  producer/publication facts before they can safely require shared authority.
- Blocked on ownership design: select-carrier/select-alias routes and
  predecessor-edge consumed suppression need a separate freshness-vs-alias
  contract; destination fan-in legality and suppression authorization must not
  be treated as source freshness.
- Split-worthy: stack-destination register fan-in and select-edge producer
  placement should be handled in a separate idea if they need freshness,
  because their current contract is destination/placement legality rather than
  direct edge-publication source acceptance.

## Suggested Next

Start Step 2 from `plan.md`: state and encode the ownership rule for
`prepare_block_entry_parallel_copy_edge_source_facts`. Prefer a narrow
direct-edge-publication source use kind unless `MoveBundleSource` can be shown
not to overload move-bundle consumer semantics; destination-only authority must
remain insufficient.

## Watchouts

- Do not claim progress through expectation rewrites, unsupported-marker
  edits, allowlist changes, named-testcase shortcuts, diagnostic-only changes,
  or backend-local ordering tweaks.
- Do not treat destination-bundle legality or structurally complete source
  homes as source freshness.
- Add or reuse only the narrowest freshness use/source vocabulary that matches
  direct edge-publication source ownership.
- Preserve or strengthen fail-closed behavior for missing, ambiguous, stale,
  wrong-value, wrong-use, destination-only, or freshness-less source authority.
- Keep closure inventory here as consumers are audited, migrated, protected,
  blocked, deferred, or split.

## Proof

Delegated proof passed:
`cmake --build --preset default > test_after.log 2>&1`.
Proof log: `test_after.log`.
