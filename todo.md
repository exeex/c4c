Status: Active
Source Idea Path: ideas/open/588_shared_prealloc_move_operand_source_freshness_inventory.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Shared-Prealloc Freshness Consumers

# Current Packet

## Just Finished

Step 1 audit completed for shared-prealloc freshness consumers.

Audited consumers and current freshness state:

- Prepared object move-bundle source consumer:
  `classify_prepared_object_move_bundle_consumer` in
  `src/backend/prealloc/prepared_object_traversal.cpp` already queries
  `find_prepared_value_freshness_authority` with
  `PreparedValueFreshnessUseKind::MoveBundleSource`. It publishes direct-home
  source freshness from complete source homes when `value_home_lookups` are
  supplied, rejects missing, invalid, ambiguous, wrong-kind, or
  destination-only move-bundle freshness, and is covered by
  `backend_prepared_object_consumer_contract_test.cpp`.
- Store-source / edge producer publication operand:
  `publish_store_source_producer_freshness_authority` in
  `src/backend/prealloc/publication_plans.cpp` already publishes and queries
  `PreparedValueFreshnessUseKind::ProducerPublicationOperand` from
  same-block source producer metadata. Prepared printer coverage already
  asserts `source_freshness_status=selected`.
- Dependency operand authorities:
  `plan_prepared_dependency_operand_authority` has the closest unwired
  `missing_stack_freshness` vocabulary. The `LoadFromStackSlot` policy accepts
  only when `stack_slot_fresh_at_edge` and clobber safety are true, while the
  collector currently records both a cast-rematerialization candidate and a
  stack-load candidate but does not publish/query shared freshness authority.
  Tests and dumps already cover the stack-load route failing closed with
  `missing_stack_freshness`.
- Branch stack-load authorities:
  `plan_prepared_branch_stack_load_authority` also has
  `missing_stack_freshness`, but the collector currently records branch
  condition/lhs/rhs stack homes with `PreparedBranchStackLoadPolicy::None` and
  local pointer-status checks. It needs a branch-use freshness contract before
  migration, so it is not the smallest first slice.
- Edge-publication move consumers:
  edge publication lookups and move-bundle traversal already provide partial
  protection through move-bundle source freshness and publication status, but
  direct edge-publication move consumption still needs a separate contract for
  whether publication source freshness, move source freshness, or destination
  bundle authority owns the use.
- Typed stack-source / aggregate stack-source publications:
  `prepare_aggregate_stack_source_authority` and typed stack-source helpers
  have concrete stack-source metadata and fail-closed statuses such as
  `incomplete_concrete_stack_source`, `unsupported_move_authority`, and
  `missing_aggregate_copy_authority`, but they do not yet expose a shared
  freshness producer for the source stack slot.
- Select-carrier / select-alias authority:
  `plan_prepared_select_carrier_alias_authority` proves source-use closure for
  select-materialized binary edge publications and has printer coverage, but
  it lacks a freshness use-kind contract distinguishing alias closure from
  source freshness.

Selected migration subset:

- Migrate dependency operand `LoadFromStackSlot` source freshness first. It is
  the smallest useful shared-prealloc subset because it already has
  `missing_stack_freshness`, a narrow source value (`dependency_home` /
  dependency operand), a concrete publication context, and focused
  stack-layout plus prepared-printer anchors. The next implementation packet
  should publish/query freshness for the dependency operand stack source
  rather than toggling the existing local `stack_slot_fresh_at_edge` boolean.

Deferred or blocked families:

- Move-bundle source consumer: already protected by the idea 587 shared query;
  keep it as a regression anchor, not a migration target.
- Store-source producer publication operand: already protected by the idea 587
  shared query; keep prepared-printer freshness rows as regression anchors.
- Branch stack-load authority: deferred until the dependency-operand route
  proves the shared stack-source freshness contract, then revisit branch
  condition/lhs/rhs use kinds and branch-point publication facts.
- Direct edge-publication move consumers: deferred because the ownership split
  between publication source freshness, move source freshness, and destination
  bundle authority needs contract design.
- Typed stack-source / aggregate stack-source publications: blocked on a
  missing shared freshness producer/publication fact for concrete stack-source
  copies; current authority is structural, not freshness authority.
- Select-carrier / select-alias authority: blocked on contract design; alias
  closure evidence is adjacent but is not itself freshness proof.

## Suggested Next

Start Step 2 for the selected dependency operand `LoadFromStackSlot` subset:
publish a shared freshness authority candidate from existing dependency
operand stack-source semantics, preserving `missing_stack_freshness` when no
valid producer/publication fact exists.

## Watchouts

- Do not claim progress through expectation rewrites, unsupported-marker edits,
  allowlist changes, named-testcase shortcuts, or target-local ordering tweaks.
- Publish freshness authority only from existing semantic prepared facts.
- Preserve or strengthen fail-closed behavior for missing, ambiguous, invalid,
  stale, wrong-use, or destination-only source authority.
- Dependency operand migration must not treat the destination edge publication
  or binary source producer as proof that the dependency stack slot is fresh.
  It needs an explicit source-side freshness candidate for the dependency
  value/use.
- If no existing dependency operand stack-source semantic fact can publish that
  candidate, stop and report a missing producer/publication fact instead of
  manufacturing freshness from the prepared home alone.
- Branch stack-load looks similar but is broader because branch condition/lhs/rhs
  roles need a branch-point use-kind contract.

## Proof

No build required by packet. Proof was read-only inspection with `rg`,
targeted source/test reads, and AST caller checks via `c4c-clang-tool-ccdb`;
no `test_after.log` was generated for this audit-only packet.
