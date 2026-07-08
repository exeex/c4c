Status: Active
Source Idea Path: ideas/open/588_shared_prealloc_move_operand_source_freshness_inventory.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Strengthen Diagnostics And Debug Visibility

# Current Packet

## Just Finished

Step 4 strengthened prepared-dump visibility for dependency operand source
freshness without changing authority construction or acceptance semantics.

`prepared-dependency-operand-authorities` rows now print the dependency
authority `source_freshness_status`, source-freshness candidate count, and the
selected authority fields when an authority is present. The existing dump
fixture asserts both the available cast-rematerialization row and the
fail-closed `LoadFromStackSlot` row expose `source_freshness_status=no_candidate`
with zero candidates, keeping missing dependency stack freshness
distinguishable in prepared dumps.

## Suggested Next

Supervisor should decide whether the active runbook is now exhausted enough for
plan-owner review/closure, or whether a narrow follow-up should make collected
dependency operand dump records carry selected stack-source freshness facts.

## Watchouts

- Do not claim progress through expectation rewrites, unsupported-marker edits,
  allowlist changes, named-testcase shortcuts, or target-local ordering tweaks.
- Step 3 kept the Step 2 vocabulary:
  `ProducerPublicationOperand` freshness use kind and `DirectHome` source kind.
  The route is now gated by selected authority shape instead of adding a new
  value-location vocabulary item.
- The collector still records the dump fixture stack-load route as
  `missing_stack_freshness`; it has no separate dependency stack-source
  producer/publication fact yet. Do not manufacture freshness from a prepared
  home alone.
- The new optional candidate input is for explicit shared-freshness candidates
  and focused contract tests; production default publication remains the
  dependency stack-home candidate created from existing semantic facts.
- Branch stack-load remains deferred; it needs its own branch-point use-kind
  contract.
- The current dependency operand collector still creates dump records without
  passing stack freshness and clobber-safety facts into the `LoadFromStackSlot`
  authority query, so this packet could only add selected-authority printing
  for authorities that already carry a selected freshness result.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_stack_layout|backend_prepared_printer|backend_prepared_lookup_helper)$'`
passed. Canonical proof log: `test_after.log`.

# Closure Inventory

## Step 1 Audited Consumers

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
  `PreparedValueFreshnessUseKind::ProducerPublicationOperand` from same-block
  source producer metadata. Prepared printer coverage already asserts
  `source_freshness_status=selected`.
- Dependency operand authorities:
  `plan_prepared_dependency_operand_authority` has the closest migrated
  `missing_stack_freshness` vocabulary. Step 2 now publishes source freshness
  for explicit `LoadFromStackSlot` planner inputs only after existing stack
  freshness and clobber-safety facts are both present. The collector still
  records the dump fixture stack-load route as `missing_stack_freshness`
  because it has no separate dependency stack-source producer/publication fact
  yet.
- Branch stack-load authorities:
  `plan_prepared_branch_stack_load_authority` also has
  `missing_stack_freshness`, but the collector currently records branch
  condition/lhs/rhs stack homes with `PreparedBranchStackLoadPolicy::None` and
  local pointer-status checks. It needs a branch-use freshness contract before
  migration.
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
  select-materialized binary edge publications and has printer coverage, but it
  lacks a freshness use-kind contract distinguishing alias closure from source
  freshness.

## Selected Migration Subset

- Dependency operand `LoadFromStackSlot` source freshness remains the selected
  migration subset. It is the smallest useful shared-prealloc subset because it
  has `missing_stack_freshness`, a narrow source value (`dependency_home` /
  dependency operand), a concrete publication context, and focused stack-layout
  plus prepared-printer anchors.

## Deferred Or Blocked Families

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
