# Direct Edge-Publication Move Freshness Ownership Runbook

Status: Active
Source Idea: ideas/open/589_direct_edge_publication_move_freshness_ownership.md

## Purpose

Settle freshness ownership for direct edge-publication move consumers, then
migrate one bounded representative route to the shared freshness authority
model introduced by idea 587.

Goal: audit direct edge-publication move consumers, state which source
freshness authority owns their proof, wire one useful consumer through the
shared query, and prove missing, ambiguous, stale, wrong-value, wrong-use, or
destination-only authority fails closed.

## Core Rule

Do not treat destination-bundle legality, structurally complete source homes,
backend-local ordering, expectation rewrites, unsupported-marker edits, or
diagnostic-only changes as freshness progress. A migrated consumer must accept
a source only through an explicit source freshness authority that matches the
direct edge-publication use.

## Read First

- ideas/open/589_direct_edge_publication_move_freshness_ownership.md
- ideas/closed/587_prepared_value_freshness_authority_mvp.md
- ideas/closed/588_shared_prealloc_move_operand_source_freshness_inventory.md
- src/backend/prealloc/value_locations.hpp
- src/backend/prealloc/prepared_lookups.cpp
- src/backend/prealloc/publication_plans.cpp
- src/backend/prealloc/prepared_object_traversal.cpp
- tests/backend/bir/backend_prepared_lookup_helper_test.cpp
- tests/backend/bir/backend_prepare_stack_layout_test.cpp
- tests/backend/bir/backend_prepared_printer_test.cpp
- tests/backend/bir/backend_prepared_object_consumer_contract_test.cpp

## Current Targets

- Direct edge-publication move consumers that currently consume publication
  rows, move bundles, source homes, or destination bundle facts without a
  shared source freshness query.
- The 587 freshness authority model:
  `PreparedValueFreshnessAuthority`,
  `PreparedValueFreshnessUseKind`,
  `PreparedValueFreshnessSourceKind`,
  `PreparedValueFreshnessProofKind`,
  `PreparedValueFreshnessSourceRank`,
  `PreparedValueFreshnessQuery`, and
  `find_prepared_value_freshness_authority`.
- The direct edge-publication route with the clearest prepared publication
  row, source home, and fail-closed diagnostic surface.

## Non-Goals

- Do not migrate every edge-publication, move-bundle, RV64, AArch64, or x86
  consumer.
- Do not redesign the move scheduler, parallel-copy legality, stack
  destination fan-in taxonomy, select-carrier aliasing, or typed aggregate
  stack-source producer facts.
- Do not replace prepared value-home lookups outside the selected
  representative edge-publication route.
- Do not change target ABI classification, `TargetProfile`, or physical
  register identity policy.
- Do not claim progress through expectation rewrites, unsupported-marker
  edits, allowlist edits, or runtime output changes.
- Do not add target-specific backend behavior unless the selected shared
  authority migration requires a small consumer adaptation.

## Working Model

The active question is source-freshness ownership for direct
edge-publication moves. Execution must determine whether the trusted proof
comes from publication-source freshness, move-source freshness, a linked
publication-to-move authority, or a new narrow use kind. Destination-only
bundle authority and structural source completeness are necessary context at
most; they are not source freshness by themselves.

For each audited consumer, answer:

- What value and edge-publication use is being consumed?
- Which source kinds are semantically valid for this use?
- Which existing producer, publication, or move fact can publish source
  freshness?
- Is existing 587 vocabulary sufficient, or would using it overload an
  unrelated use kind?
- Which missing, ambiguous, stale, wrong-value, wrong-use, or
  destination-only cases must fail closed?
- Which status, diagnostic, or prepared dump makes the selected or missing
  authority observable?

Prefer one representative migration over a broad sweep. The closure inventory
is part of the deliverable.

## Execution Rules

- Begin with an audit and ownership rule before changing consumer behavior.
- Keep each executor packet narrow enough for build plus focused prepared or
  backend tests.
- Add or reuse only the narrowest freshness use/source vocabulary needed for
  the selected direct edge-publication move route.
- Publish and query freshness authority from semantic prepared facts, not from
  destination-only legality or testcase shape.
- Preserve fail-closed behavior for missing, ambiguous, stale, wrong-value,
  wrong-use, destination-only, and structurally complete but freshness-less
  sources.
- Track closure-inventory notes in `todo.md` as consumers are audited,
  migrated, deferred, blocked, or split.
- Escalate to supervisor/reviewer if a packet changes only expectations,
  unsupported markers, allowlists, diagnostic strings, or backend-local
  fallback ordering.
- Use build proof plus the focused test subset selected by the supervisor for
  each implementation packet; broader regression proof belongs to supervisor
  acceptance or close.

## Step 1: Audit Direct Edge-Publication Move Consumers

Goal: identify the direct edge-publication move consumer set and select the
smallest representative route for migration.

Primary targets:

- src/backend/prealloc/publication_plans.cpp
- src/backend/prealloc/prepared_object_traversal.cpp
- src/backend/prealloc/prepared_lookups.cpp
- src/backend/prealloc/value_locations.hpp
- tests/backend/bir/backend_prepare_stack_layout_test.cpp
- tests/backend/bir/backend_prepared_printer_test.cpp
- tests/backend/bir/backend_prepared_object_consumer_contract_test.cpp

Actions:

- Inventory direct edge-publication move consumers and nearby helper APIs that
  consume source homes, edge-publication rows, or move bundles.
- Record whether each consumer currently relies on publication rows, move
  bundles, destination legality, structural source homes, or an explicit
  freshness query.
- Identify the route with the clearest prepared publication row, source home,
  value identity, and fail-closed diagnostic surface.
- Record which consumers are already protected, blocked on missing
  producer/publication facts, blocked on ownership design, or deferred for
  scope.
- Write the audit result and selected representative route into `todo.md`.

Completion check:

- `todo.md` names audited direct edge-publication move consumers, the selected
  migration route, and why each unselected family is deferred, protected, or
  blocked.
- The selected route can be migrated without redoing the audit.
- No implementation behavior has changed except incidental compile-safe
  discovery edits, if any.

## Step 2: State And Encode The Ownership Rule

Goal: make the direct edge-publication move freshness ownership rule explicit
in shared-prealloc vocabulary and diagnostics.

Primary targets:

- src/backend/prealloc/value_locations.hpp
- src/backend/prealloc/prepared_lookups.cpp
- the selected publication or move helper from Step 1
- focused prepared dump or helper tests for freshness vocabulary visibility

Actions:

- Decide whether existing 587 use/source kinds are semantically correct for
  the selected route.
- If existing vocabulary would overload an unrelated use, add a narrowly named
  direct edge-publication freshness use kind.
- Define which source kinds and proof ranks are valid for the selected use.
- State in `todo.md` why publication-source freshness, move-source freshness,
  or a linked publication-to-move authority owns the proof.
- Keep destination-only authority explicitly insufficient.

Completion check:

- The ownership rule is visible in code, test names, diagnostic text, or
  `todo.md` notes before consumer acceptance changes rely on it.
- Any added use/source vocabulary is narrow to direct edge-publication source
  ownership.
- Existing 587 and 588 freshness tests continue to pass.

## Step 3: Publish Or Locate Freshness For The Selected Route

Goal: ensure the selected direct edge-publication move route has explicit
source freshness authority available from semantic prepared facts.

Primary targets:

- the selected publication-source, move-source, or linked authority producer
  from Step 1
- src/backend/prealloc/prepared_lookups.cpp if query matching needs a narrow
  extension
- focused lookup or prepared dump tests for the selected authority

Actions:

- Publish or reuse authority candidates for the selected value/use from
  existing semantic prepared facts.
- Record source reference, source kind, proof kind, and rank precisely enough
  for the shared query to reject invalid or ambiguous candidates.
- Ensure freshness-less or destination-only facts do not create authority.
- Add focused coverage proving accepted explicit authority is visible.

Completion check:

- The selected route's source freshness authority is available to the shared
  query.
- Missing source facts, destination-only facts, and incomplete source homes do
  not produce accepted authority.
- Focused tests or dumps prove authority visibility without weakening
  expectations.

## Step 4: Wire The Selected Consumer To The Freshness Query

Goal: make one representative direct edge-publication move consumer require
the selected source freshness authority before accepting its source.

Primary targets:

- the selected direct edge-publication move consumer from Step 1
- src/backend/prealloc/prepared_lookups.cpp if the query result needs a narrow
  status mapping
- focused consumer contract tests for the migrated route

Actions:

- Replace implicit source acceptance with a shared freshness query for the
  selected value/use.
- Accept only authority with the expected use kind, source kind, proof kind,
  source reference, and freshness ordering.
- Fail closed for missing, ambiguous, stale, wrong-value, wrong-use,
  destination-only, and structurally complete but freshness-less sources.
- Preserve target-independent checks for payload completeness, storage class,
  publication row identity, and move legality.
- Keep diagnostics specific enough to distinguish missing freshness from
  unsupported shape and local target incompleteness.

Completion check:

- At least one direct edge-publication move route consults shared freshness
  authority before accepting a source.
- Focused tests prove accepted explicit source freshness and rejected implicit,
  destination-only, stale, ambiguous, wrong-value, or wrong-use authority.
- The diff is semantic and not testcase-shaped matching.

## Step 5: Strengthen Observability And Closure Inventory

Goal: make the migrated route inspectable and leave the remaining
edge-publication move map ready for closure.

Primary targets:

- prepared printer, verifier, or diagnostic surfaces touched by the selected
  route
- focused prepared dump tests for the selected route
- `todo.md` closure inventory notes

Actions:

- Ensure selected source freshness authority is printable or otherwise
  assertable in focused tests.
- Ensure fail-closed statuses distinguish unsupported shape, missing source
  freshness, ambiguous source freshness, stale/wrong-use freshness, and local
  target incompleteness where the selected route exposes those cases.
- Recheck adjacent direct edge-publication move consumers from Step 1 and
  classify each as migrated, protected, blocked, deferred, or ready for a
  follow-up idea.
- Record any newly exposed architecture gap or target-specific RV64, AArch64,
  or x86 tail in `todo.md`.

Completion check:

- Focused dumps or diagnostics identify the migrated source authority and the
  relevant failure categories.
- `todo.md` contains the closure-note facts required by the source idea for
  audited, migrated, unwired, newly exposed, and recommended-follow-up
  families.
- No unrelated diagnostic churn is used as proof of progress.

## Step 6: Recheck Freshness Regression Anchors

Goal: prove the direct edge-publication migration did not regress existing
freshness authority behavior from ideas 587 and 588.

Primary targets:

- tests/backend/bir/backend_prepared_lookup_helper_test.cpp
- tests/backend/bir/backend_prepared_printer_test.cpp
- tests/backend/bir/backend_prepared_object_consumer_contract_test.cpp
- tests covering the selected direct edge-publication move route

Actions:

- Run the focused test subset for the migrated route and existing 587/588
  freshness consumers.
- Confirm fail-closed cases still reject missing, ambiguous, stale,
  wrong-value, wrong-use, and destination-only authority.
- Update `todo.md` with exact proof commands and results.
- Request a separate source idea if execution discovers a distinct broader
  contract change instead of absorbing it into this plan.

Completion check:

- Focused proof is green for the migrated direct edge-publication route and
  existing 587/588 freshness anchors.
- `todo.md` contains proof commands, results, remaining consumer inventory,
  and any follow-up recommendation needed for closure.
- The plan is ready for supervisor review or close-time regression gating.
