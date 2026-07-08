# Direct Edge-Publication Move Freshness Ownership

Status: Closed
Type: Architecture contract and narrow consumer migration
Parent: `ideas/closed/588_shared_prealloc_move_operand_source_freshness_inventory.md`
Related:
- `ideas/closed/585_target_abi_contract_and_value_consumption_research.md`
- `ideas/closed/587_prepared_value_freshness_authority_mvp.md`
- `ideas/closed/588_shared_prealloc_move_operand_source_freshness_inventory.md`
Owning Layer: shared-prealloc edge-publication move consumers, freshness
authority ownership, and move/source/destination contract boundaries

## Goal

Define who owns freshness proof for direct edge-publication move consumers, and
migrate one bounded representative route to that ownership contract.

The key question is not merely how to make an edge-publication move pass. The
implementation must answer whether the trusted source authority comes from
publication-source freshness, move-source freshness, destination-bundle
authority, or an explicit link between them. Once that ownership is stated,
wire the smallest useful consumer slice to the 587 freshness authority model.

## Why This Exists

Idea 587 made move-bundle source freshness first-class for representative
prepared object consumers. Idea 588 migrated dependency operand
`LoadFromStackSlot` through the shared freshness query and left direct
edge-publication move consumers unwired.

The remaining ambiguity is architectural:

- an edge-publication row can prove that a value should be published across an
  edge;
- a move bundle can prove that a destination is legal or that a bundle shape is
  classified;
- a source home can be structurally complete;
- none of those facts alone necessarily proves that the move source is fresh
  for this edge-publication use.

This idea should settle that ownership boundary for one representative direct
edge-publication move route and leave a clear inventory for the rest.

## In Scope

- Audit direct edge-publication move consumers and nearby helper APIs that
  currently consume source homes, edge-publication rows, or move bundles
  without consulting shared source freshness.
- State the ownership rule for direct edge-publication moves:
  - which use kind is queried;
  - which source kinds are accepted;
  - whether publication-source freshness, move-source freshness, or a linked
    publication-to-move authority owns the proof;
  - which destination-only authorities are explicitly insufficient.
- Add or reuse the narrowest freshness use/source vocabulary needed for the
  selected route. If the 587 vocabulary is sufficient, use it directly; if it
  is not, add a narrowly named use kind rather than overloading an unrelated
  one.
- Migrate one representative direct edge-publication move consumer to require
  selected freshness authority before accepting the source.
- Preserve fail-closed behavior for missing, ambiguous, stale, wrong-value,
  wrong-use, destination-only, or structurally complete but freshness-less
  source routes.
- Add focused tests and/or prepared dump assertions proving the migrated route
  accepts explicit source freshness and rejects implicit or destination-only
  authority.
- Keep target-specific backend changes out of the slice unless the migrated
  shared-prealloc authority requires a small consumer adaptation.

## Out Of Scope

- Full migration of every edge-publication, move-bundle, RV64, AArch64, or x86
  consumer.
- Redesigning the move scheduler, parallel-copy legality, or stack-destination
  fan-in taxonomy.
- Treating destination bundle legality as source freshness.
- Broadly replacing prepared value-home lookups outside the selected
  representative edge-publication route.
- Changing target ABI classification, `TargetProfile`, or physical register
  identity policy.
- Expectation rewrites, unsupported-marker edits, allowlist edits, or runtime
  output changes as proof of progress.

## Acceptance Criteria

- The idea identifies the audited direct edge-publication move consumer set.
- The implementation states a concrete freshness ownership rule for direct
  edge-publication moves.
- At least one direct edge-publication move route consults the shared
  freshness authority before accepting a source.
- The migrated route rejects source-less, freshness-less, destination-only,
  stale, ambiguous, wrong-value, or wrong-use authority with precise
  fail-closed status or diagnostics.
- Focused tests or prepared dumps prove the accepted route is authorized by
  explicit source freshness rather than a complete destination bundle or old
  prepared home.
- Existing 587 and 588 freshness tests continue to pass.
- Any discovered need for a broader contract change is recorded and split
  rather than hidden inside a broad local rewrite.

## Closure Note Requirements

Do not close this idea with a generic "future work remains" note. The closure
note must answer:

1. Which direct edge-publication move consumers were audited?
2. What is the ownership rule for source freshness in direct
   edge-publication moves?
3. Which freshness use kinds and source kinds were used or added?
4. Which representative consumer was migrated?
5. Which missing, ambiguous, stale, wrong-value, wrong-use, or
   destination-only cases now fail closed, and through what diagnostics or
   statuses?
6. Which direct edge-publication move consumers remain unwired, and are they
   already protected, blocked on missing producer/publication facts, blocked
   on contract design, or simply deferred for scope?
7. Did the work expose a new architecture gap not already covered by ideas
   585, 587, or 588?
8. Did the work expose any target-specific RV64, AArch64, or x86 tail that is
   ready for a follow-up idea?
9. What concrete follow-up ideas should be opened next, if any?

## Recommended First Cut

Start from the direct edge-publication move path that already has the clearest
prepared publication row, source home, and fail-closed diagnostic surface.
Prefer a route where the implementation can prove "source freshness is
required" without simultaneously redesigning select-carrier aliasing, typed
aggregate stack sources, or target backend emission.

If the audit shows there is no safe representative route without first adding
a new freshness use kind, make that use kind narrow and explicit, for example
an edge-publication source use, and prove why `MoveBundleSource` or
`ProducerPublicationOperand` would be semantically overloaded.

## Reviewer Reject Signals

- Reject a slice that accepts a source because the destination bundle is valid
  while source freshness is absent.
- Reject overloading an unrelated freshness use kind to avoid naming the
  edge-publication ownership boundary.
- Reject backend-local ordering or fallback tweaks presented as shared
  edge-publication freshness ownership.
- Reject a route that only changes diagnostics or expectations without moving
  a real consumer to freshness authority.
- Reject closure notes that do not separately list audited, migrated,
  unwired, newly exposed, and recommended-follow-up families.

## Closure Note

Closed after completing the representative direct edge-publication move
freshness route and backend close gate.

Audited consumers:

- `make_prepared_edge_publication_lookups` builds direct edge-publication rows
  from join transfers, destination homes, source homes, source producer facts,
  and matching moves. It remains a row publisher/helper, not the migrated
  acceptance consumer.
- `prepare_edge_copy_source_facts`,
  `find_unique_indexed_block_entry_parallel_copy_edge_publication`, and
  `prepare_block_entry_parallel_copy_edge_source_facts` consume indexed
  edge-publication rows and block-entry out-of-SSA move facts.
- `prepare_current_block_join_parallel_copy_source_facts` consumes those edge
  source facts for current-block join parallel-copy source acceptance.
- `prepare_aggregate_stack_source_authority` and
  `prepare_same_width_i32_stack_source_publication` consume publication rows,
  stack source homes, destination register placement, and matching moves for
  stack-source publication routes.
- `classify_prepared_object_move_bundle_consumer` consumes move bundles through
  `PreparedValueFreshnessUseKind::MoveBundleSource`.
- `plan_prepared_store_source_publication`,
  `publish_store_source_producer_freshness_authority`, and
  `plan_prepared_dependency_operand_authority` consume producer-publication
  operands through `PreparedValueFreshnessUseKind::ProducerPublicationOperand`.
- Select-carrier placement, select-alias collection, destination fan-in, and
  predecessor-edge consumed suppression routes consume edge-publication rows or
  nearby move facts for placement, alias, suppression, or destination legality.

Ownership rule:

Direct edge-publication moves require explicit source freshness for the exact
edge-publication source being accepted. Destination bundle legality, complete
source homes, direct homes, alias evidence, and local move shape are not source
freshness. The accepted proof is the exact direct edge-publication row linked
to the exact block-entry parallel-copy move resolution for the same source
value and edge.

Freshness vocabulary:

- Added `PreparedValueFreshnessUseKind::DirectEdgePublicationSource`.
- Added `PreparedValueFreshnessSourceKind::DirectEdgePublication`.
- Added `PreparedValueFreshnessProofKind::DirectEdgePublicationMove`.
- Added `PreparedValueFreshnessSourceRank::DirectEdgePublication`.
- Added an `edge_publication` freshness reference alongside the exact move
  reference.

Migrated representative consumer:

The migrated family is the current-block join parallel-copy source route
through `prepare_block_entry_parallel_copy_edge_source_facts` and
`prepare_current_block_join_parallel_copy_source_facts`. The lower helper
publishes selected direct edge-publication source freshness after the existing
publication, move, and source-value checks succeed. The current-block join
consumer requires the selected `DirectEdgePublicationSource` authority before
accepting named direct edge-publication sources.

Fail-closed coverage:

Focused backend helper tests cover accepted explicit freshness and rejection
for missing or no-candidate freshness, invalid freshness, ambiguous freshness,
wrong source value, wrong use, and destination-only authority. Existing
missing publication, ambiguous publication, edge mismatch, unsupported move,
publication/move mismatch, and missing source-fact checks remain preserved.
Immediate edge sources intentionally remain authority-free and are not
retrofitted with freshness candidates.

Observability:

The prepared printer exposes
`prepared-current-block-join-parallel-copy-sources` rows derived through
`prepare_current_block_join_parallel_copy_source_facts`. Rows show edge/source
status, source freshness query status, candidate count, selected authority
source kind, source value/id, use, proof, rank, reference block/instruction,
source/destination home classes, immediate-source state, and Route 5 agreement
status.

Remaining unwired families:

- Protected: object traversal move-bundle consumers remain covered by
  `MoveBundleSource`; dependency and store-source producer-publication
  consumers remain covered by the existing 587/588
  `ProducerPublicationOperand` freshness surfaces.
- Deferred for scope: `make_prepared_edge_publication_lookups`, broad
  RV64/AArch64/x86 emission tails, and non-selected edge-publication or
  move-bundle consumer families.
- Blocked on missing producer/publication facts: typed stack-source and
  aggregate stack-source publication routes.
- Blocked on ownership design and split-worthy if continued: select-carrier
  alias, destination fan-in, predecessor-edge consumed suppression, and
  similar destination/alias legality routes.

New gaps and follow-ups:

No new target-specific RV64, AArch64, or x86 tail was exposed by the completed
representative migration. Any follow-up should be a separate idea focused on
select/alias freshness ownership or typed/aggregate stack-source freshness,
if the supervisor chooses to continue beyond this representative route.

Close proof:

- Backend close logs: `test_before.log` and `test_after.log` both record 346
  passing `^backend_` tests and 0 failures.
- Regression guard passed in non-decreasing mode for this lifecycle-only close:
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`.
