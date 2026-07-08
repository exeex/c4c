# Select-Carrier Alias Freshness Contract Runbook

Status: Active
Source Idea: ideas/open/598_select_carrier_alias_freshness_contract.md
Activated From:
- ideas/closed/595_prepared_value_architecture_followup_umbrella.md
- ideas/closed/597_pointer_address_semantic_model_research.md

## Purpose

Define the freshness-vs-alias ownership rule for select-carrier alias source
acceptance, then migrate at most one representative shared-prealloc consumer
only after that rule is explicit.

## Goal

Make select-carrier alias source acceptance depend on explicit selected source
freshness, or record why alias facts must remain support-only for this route.

## Core Rule

Do not accept a select-carrier alias source because alias metadata,
destination legality, suppression state, complete homes, or target operand
shape exists. A migrated consumer must fail closed unless the selected source
freshness contract for the exact use is satisfied.

## Read First

- ideas/open/598_select_carrier_alias_freshness_contract.md
- ideas/closed/587_prepared_value_freshness_authority_mvp.md
- ideas/closed/588_shared_prealloc_move_operand_source_freshness_inventory.md
- ideas/closed/589_direct_edge_publication_move_freshness_ownership.md
- ideas/closed/595_prepared_value_architecture_followup_umbrella.md
- ideas/closed/597_pointer_address_semantic_model_research.md
- docs/prepared_value_architecture_followup_umbrella/03_followup_idea_backlog.md
- docs/prepared_value_architecture_followup_umbrella/04_dependency_and_priority_order.md

## Current Targets

- `src/backend/prealloc/publication_plans.hpp`
- `src/backend/prealloc/publication_plans.cpp`
- `src/backend/prealloc/prealloc.cpp`
- `src/backend/prealloc/prepared_object_traversal.hpp`
- `src/backend/prealloc/prepared_object_traversal.cpp`
- `src/backend/prealloc/prepared_printer/select_chains.cpp`
- `src/backend/mir/riscv/codegen/prepared_function_emit.*`
- `src/backend/mir/riscv/codegen/prepared_edge_publication_emit.*`
- `src/backend/mir/riscv/codegen/object_emission.cpp`

## Non-Goals

- Do not solve destination fan-in authority.
- Do not solve predecessor-edge consumed suppression.
- Do not migrate broad RV64, AArch64, x86, string assembly, or other target
  consumers.
- Do not redesign parallel-copy legality, move-bundle scheduling, or producer
  publication outside the selected representative route.
- Do not fold pointer-base-plus-offset, pointer-value memory freshness, or
  Prepared MIR view design into this route.
- Do not change expectations, unsupported markers, allowlists, runtime
  behavior, or harness behavior as proof of progress.

## Working Model

- Existing select-carrier alias records may be support evidence for select
  carrier identity and join-transfer shape.
- Source freshness is use-specific and must be queried through the prepared
  freshness authority model when a consumer treats a source as semantically
  current.
- `MoveBundleSource` and `DirectEdgePublicationSource` are existing freshness
  kinds for their own routes. Reuse them only if the audited contract proves
  the select-carrier alias route is the same ownership boundary; otherwise add
  distinct vocabulary.
- Missing, ambiguous, stale, wrong-value, wrong-use, alias-only,
  destination-only, suppression-only, and structurally complete but
  freshness-less routes must fail closed.

## Execution Rules

- Keep each packet narrow and update `todo.md` with the audited consumer set,
  selected rule, proof command, and remaining follow-ups.
- Decide the contract before wiring a representative consumer.
- Prefer shared-prealloc authority and query helpers over target-local shape
  checks.
- Treat prepared dumps and diagnostics as proof surfaces, not semantic
  authority.
- If the audit discovers destination fan-in, predecessor-edge suppression,
  target migration, pointer/address work, or MIR view work is required, record
  it as a follow-up instead of expanding this plan.

## Step 1. Audit Select-Carrier Alias Consumers

Goal: identify the live select-carrier and select-alias surfaces that produce,
carry, inspect, or accept alias-shaped source evidence.

Actions:

- Inspect `PreparedSelectCarrierAliasAuthority`,
  `PreparedSelectCarrierAliasAuthorityRecords`,
  `plan_prepared_select_carrier_alias_authority(...)`,
  `collect_prepared_select_carrier_alias_authorities(...)`, and
  `populate_select_carrier_alias_identity(...)`.
- Inspect current diagnostics and dump output for select-carrier alias
  authority.
- Inventory consumers in shared-prealloc and the narrow RV64 paths only enough
  to understand which shared-prealloc route can be a representative migration.
- Record which surfaces are producers, support-fact collectors, consumers,
  diagnostics, or target-consume-only paths.

Completion Check:

- `todo.md` names the audited consumer set and one proposed representative
  shared-prealloc consumer, or explains why implementation should stop at a
  research/contract packet.

## Step 2. State The Ownership Contract

Goal: make the selected freshness rule explicit before changing acceptance.

Actions:

- State the select-carrier alias use being authorized.
- Decide which freshness use kind and source kind are required.
- Decide whether a new select-carrier alias freshness vocabulary entry is
  required or whether an existing kind is truly the same owner.
- List every fact class that remains insufficient by itself: alias-only,
  destination-only, suppression-only, complete-home, target-shape, and
  structural join-transfer evidence.
- Define expected failure statuses or diagnostics for missing, ambiguous,
  stale, wrong-value, wrong-use, and freshness-less routes.

Completion Check:

- The contract is documented in `todo.md` and, if implementation proceeds,
  reflected in code names or helper boundaries without weakening existing
  fail-closed behavior.

## Step 3. Migrate One Representative Consumer

Goal: require selected source freshness in one representative select-carrier
alias consumer after the contract is explicit.

Actions:

- Add or reuse the minimal freshness authority vocabulary required by Step 2.
- Publish or collect the authority from existing semantic prepared facts only.
- Query the selected authority before accepting the representative source.
- Reject missing, ambiguous, incomplete, wrong-value, wrong-use, alias-only,
  destination-only, and structurally complete but freshness-less evidence.
- Keep adjacent target migration and producer-publication repairs out of this
  step unless the selected representative route cannot compile without a small
  mechanical hookup.

Completion Check:

- One representative consumer accepts only when the selected authority matches
  the exact value, use, source, proof, reference, rank, and program point
  required by the contract.

## Step 4. Prove Fail-Closed Behavior

Goal: demonstrate the route accepts explicit selected source freshness and
rejects alias or destination authority by itself.

Actions:

- Add focused tests or prepared dump assertions for the migrated route.
- Cover explicit accepted freshness plus missing, ambiguous, stale,
  wrong-value, wrong-use, alias-only, destination-only, and structurally
  complete but freshness-less cases where practical.
- Keep diagnostics precise enough to distinguish missing freshness from
  unrelated unsupported route failures.
- Re-run existing 587, 588, and 589 freshness authority tests or their closest
  repo-native subset.

Completion Check:

- Fresh proof shows selected source freshness is required and alias metadata,
  destination legality, suppression facts, complete homes, and target-local
  operand shape do not authorize the source.

## Step 5. Handoff And Broader Validation

Goal: leave a clear closure path and separate any newly discovered initiatives.

Actions:

- Update `todo.md` with the closure-note answers required by the source idea.
- Record separate follow-up candidates for destination fan-in,
  predecessor-edge suppression, producer publication, target migration, or MIR
  view work if discovered.
- Run the supervisor-selected broader validation or regression guard before
  closure.

Completion Check:

- The source idea can be closed with audited consumers, ownership rule,
  freshness kinds, migrated route, fail-closed cases, proof commands, and
  separate follow-ups all named.
