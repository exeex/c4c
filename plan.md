# Route Index Facade Contraction Runbook

Status: Active
Source Idea: ideas/open/174_route_index_facade_contraction.md

## Purpose

Expand or contract the route-index reference facade only where a typed route
already owns the semantic record and can provide fail-closed reference
validation for a selected consumer boundary.

## Goal

Add at most one narrow facade-backed consumer path or contraction point without
turning the facade into a generic `PreparedFunctionLookups` replacement.

## Core Rule

The route-index facade may carry stable references to typed route records. It
must not duplicate semantic payloads, scan broad BIR structures, or become a
new lowering-plan aggregate.

## Read First

- `ideas/open/174_route_index_facade_contraction.md`
- `docs/bir_prealloc_fusion/phase_c_private_cache_contraction.md`
- `src/backend/bir/bir.hpp`
- `src/backend/bir/bir.cpp`
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`
- Existing selected Route 4 and Route 7 facade validation coverage

## Current Targets

- `bir::RouteIndexReferenceFacade`
- `bir::RouteIndexRecordReference`
- `bir::RouteIndexRecordCategory`
- `bir::RouteIndexValidationStatus`
- `bir::Route4IndexReferenceValidation`
- `bir::Route7IndexReferenceValidation`
- `bir::route_index_reference_facade(...)`
- `bir::route_index_validate_current_block_publication_reference(...)`
- `bir::route_index_validate_block_entry_publication_reference(...)`
- `bir::route_index_validate_comparison_operand_reference(...)`
- `bir::route_index_validate_materialized_condition_reference(...)`
- The selected typed route index and exactly one selected consumer boundary

## Non-Goals

- Do not replace typed route indexes wholesale.
- Do not create a BIR-owned `PreparedFunctionLookups` clone.
- Do not copy semantic payloads into route-index facade records.
- Do not move broad consumer migrations through a generic facade.
- Do not claim progress from helper renames, expectation rewrites, or facade
  reshaping alone.
- Do not weaken supported tests or mark supported paths unsupported without
  explicit approval.

## Working Model

Existing facade coverage is intentionally partial: selected Route 4 and Route 7
references have typed validators and fail-closed tests. Any expansion must
start from a typed route-owned record/index and prove reference validation
before a consumer is allowed to use the facade. If no typed validator and
consumer boundary satisfy that bar, the correct result is to record the blocker
in `todo.md`, not to broaden the facade.

## Execution Rules

- Move one reference category and one consumer boundary at a time.
- Prefer typed route validators over generic facade logic.
- Keep typed route records as the semantic owner.
- Preserve prepared lookup or typed route answers as oracles until the selected
  consumer has equivalent facade-backed proof.
- Every code-changing step needs fresh build proof plus the delegated narrow
  subset from the supervisor.
- Escalate to broader backend validation if the diff changes shared facade
  structures, public BIR headers, or multiple route validators.

## Step 1: Inventory Typed Validators And Candidate Consumer

Goal: identify whether one route-specific reference category has explicit
typed validation and a selected consumer boundary that can safely use the
facade.

Primary targets:

- `src/backend/bir/bir.hpp`
- `src/backend/bir/bir.cpp`
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`
- The selected route's existing record/index helpers

Actions:

- Inventory current facade entry points and the typed validators behind them.
- Identify candidate routes or reference categories that already have enough
  typed record/index ownership to validate references fail-closed.
- Select exactly one consumer boundary that can use the facade without
  broadening `PreparedFunctionLookups` or replacing typed route indexes.
- If no candidate satisfies the rule, record the blocker and stop.
- Record the selected category, selected consumer, and narrow proof subset in
  `todo.md`.

Completion check:

- `todo.md` names one selected reference category and consumer boundary, or it
  records that no activatable implementation packet exists under this idea.

## Step 2: Add Or Tighten Fail-Closed Typed Validation

Goal: make the selected reference category fail closed through typed route
validation before any consumer relies on the facade.

Actions:

- Add or tighten the route-specific validator for the selected typed record.
- Ensure stale owner, wrong route, wrong category, wrong relationship, missing
  record, duplicate reference, and role mismatch cases reject as applicable.
- Store only references in facade records; keep semantic facts in the typed
  route index.
- Extend focused BIR oracle tests for positive and negative validation cases.

Completion check:

- The selected reference category has focused fail-closed validation coverage.
- No semantic payload was copied into `RouteIndexRecordReference` or the
  facade.

## Step 3: Migrate One Consumer Boundary To The Facade

Goal: make exactly one selected consumer use the validated facade reference
boundary while the typed route remains the semantic owner.

Actions:

- Replace only the selected consumer's direct typed-reference access with the
  validated facade boundary.
- Keep typed route records and prepared lookup answers available as oracles or
  compatibility paths where still needed.
- Do not introduce broad BIR scans or generic aggregate lookup wiring.
- Add or update the exact consumer subset that observes the migrated boundary.

Completion check:

- The selected consumer uses the facade only after typed validation succeeds.
- Nearby consumers still use their existing typed route or prepared oracle
  paths.
- The narrow facade validation and selected consumer subsets pass.

## Step 4: Contract Only Redundant Facade Or Prepared Exposure

Goal: remove or narrow only the exposure made redundant by the selected
validated consumer path.

Actions:

- Inspect remaining uses of the old direct path, prepared helper, or facade
  shim before deleting or privatizing anything.
- Contract only the surface that the migrated consumer no longer needs.
- Leave `PreparedFunctionLookups`, direct lookup pointers, and typed route
  indexes public wherever remaining consumers still require them.
- Update tests only to reflect the real ownership boundary; do not weaken
  behavior.

Completion check:

- The contracted surface is unused by the migrated consumer.
- Remaining consumers still have the required typed route or migration-oracle
  access.

## Step 5: Acceptance Validation

Goal: prove the slice is narrow facade contraction, not aggregate replacement
or testcase-shaped progress.

Actions:

- Run the delegated focused BIR facade validation subset.
- Run the exact migrated consumer subset.
- Run build proof for the touched target.
- Escalate to broader backend validation if shared BIR facade structures,
  public headers, or multiple validators changed.

Completion check:

- Fresh proof logs show fail-closed validation and the selected consumer subset
  are green.
- No generic lowering-plan aggregate, broad BIR scan, semantic payload copy, or
  unsupported expectation downgrade was introduced.
