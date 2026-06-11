# BIR Return-Chain Prepared API Contraction

Status: Active
Source Idea: ideas/open/180_bir_return_chain_prepared_api_contraction.md

## Purpose

Contract the prepared return-chain API now that the BIR-owned return-chain route
is the semantic owner and target consumers no longer need public prepared
helpers.

## Goal

Remove, hide, or narrow prepared return-chain helper surface without changing
return-chain semantics or weakening proof coverage.

## Core Rule

Do not contract prepared public API until the BIR/prepared oracle equivalence
and AArch64 consumer migration assumptions are freshly checked.

## Read First

- `ideas/open/180_bir_return_chain_prepared_api_contraction.md`
- Recent closed prerequisite: `ideas/closed/179_bir_return_chain_consumer_migration.md`
- Prepared return-chain lookup declarations and aggregate projection fields
- BIR route/schema tests and prepared/BIR oracle equivalence tests
- Focused AArch64 return-chain lowering tests

## Current Targets

- Public prepared return-chain helper visibility.
- Prepared lookup aggregate fields that only expose return-chain terminal or
  next-operand identity.
- Tests or diagnostics that still reference prepared return-chain helpers as a
  migration oracle.

## Non-Goals

- Do not add or redesign the BIR schema/index.
- Do not add the initial oracle equivalence suite.
- Do not migrate AArch64 consumers as part of this contraction.
- Do not change accepted or rejected return-chain semantics.
- Do not broaden accepted return-chain cases.

## Working Model

- BIR is the semantic route for terminal and next-operand answers.
- Prepared return-chain helpers may remain internal implementation detail only
  when another bounded route still owns that dependency.
- Public prepared projection fields that exist only for migration should be
  removed or narrowed once no active consumer relies on them.

## Execution Rules

- Keep the contraction separate from new schema behavior or consumer migration.
- Prefer deleting or hiding obsolete public prepared API over renaming it behind
  equivalent public exposure.
- Retarget tests to the BIR-owned route when they still assert semantic
  terminal or next-operand behavior.
- Do not weaken, delete, or mark supported-path tests unsupported to make the
  cleanup pass.
- If a prepared helper is still required, document the concrete consumer in
  `todo.md` and stop before broadening this plan.
- Escalate to broader backend validation when public headers or prepared
  aggregate projection shape changes.

## Ordered Steps

### Step 1: Reconfirm Prerequisite Proof

Goal: establish that the planned contraction is allowed.

Primary targets:

- Oracle equivalence tests covering prepared and BIR return-chain answers.
- Focused AArch64 return-chain lowering tests.
- Any prepared lookup API tests that guard migration-era access.

Actions:

- Inspect the current test names and command shape for the oracle equivalence
  and focused AArch64 return-chain suites.
- Run or delegate the supervisor-selected prerequisite proof command before
  making API contraction edits.
- Check that AArch64 consumers use the BIR-owned route for semantic terminal
  and next-operand answers.
- Record exact proof commands and results in `todo.md`.

Completion check:

- Fresh proof confirms BIR/prepared equivalence and migrated AArch64 consumer
  behavior, or the plan is blocked with the missing prerequisite named in
  `todo.md`.

### Step 2: Inventory Prepared Public Surface

Goal: identify every prepared return-chain helper and projection field that is
eligible for contraction.

Primary targets:

- Public prepared lookup headers.
- Prepared return-chain aggregate fields.
- Diagnostics or tests that still include prepared helper names.

Actions:

- List public helpers and aggregate fields that expose terminal or
  next-operand identity.
- Classify each item as public obsolete surface, required internal detail, or
  still-owned dependency.
- Search consumers before editing so no live code path is broken by surprise.
- Record any retained internal dependency in `todo.md` with its concrete owner.

Completion check:

- `todo.md` contains the contraction inventory and no candidate is removed
  without a known consumer classification.

### Step 3: Contract Eligible Prepared API

Goal: remove, hide, or narrow the obsolete public prepared return-chain surface.

Primary targets:

- Prepared lookup declarations and definitions.
- Aggregate projection fields used only for migration-era terminal or
  next-operand exposure.
- Internal callers that need adjusted access after visibility changes.

Actions:

- Remove public declarations for helpers that no active consumer needs.
- Narrow projection aggregates by deleting fields that only expose
  return-chain terminal or next-operand identity.
- Keep bounded internal implementation detail private when another route still
  depends on it.
- Build after each coherent contraction slice.

Completion check:

- Code builds and no public prepared helper or projection field remains solely
  to answer semantic terminal or next-operand queries.

### Step 4: Retarget Tests And Diagnostics

Goal: keep coverage semantic while removing migration-oracle dependency on the
prepared route.

Primary targets:

- Prepared lookup API tests touched by the contraction.
- Oracle equivalence tests.
- Focused AArch64 return-chain lowering tests.
- Diagnostics that expose prepared helper wording.

Actions:

- Retarget semantic assertions to the BIR-owned route.
- Preserve oracle tests that are still needed to prove equivalence, but do not
  keep public prepared helper access only for diagnostics.
- Update expected diagnostics only when the user-facing API or wording truly
  changed.
- Reject expectation downgrades that reduce supported-path coverage.

Completion check:

- Touched tests assert the BIR route or retained internal prepared behavior as
  appropriate, with no weakened contracts.

### Step 5: Validate Contraction Boundary

Goal: prove the cleanup did not change return-chain behavior or hide a missing
consumer migration.

Primary targets:

- Build.
- Oracle equivalence suite.
- Focused AArch64 return-chain lowering tests.
- Prepared lookup API tests touched by the cleanup.
- Broader backend validation if headers or aggregate projection shape changed.

Actions:

- Run the supervisor-selected narrow proof after the contraction.
- Compare failures against the pre-edit prerequisite proof.
- Escalate to broader backend validation when the edited surface is public or
  shared across backend buckets.
- Record final proof commands and results in `todo.md`.

Completion check:

- Fresh validation is green for the required scope, and any broader validation
  required by public API shape changes has been run or explicitly left for the
  supervisor with rationale in `todo.md`.

## Completion Criteria

- Prepared return-chain public helpers and projection fields are contracted to
  the minimum surface still required by bounded internal implementation.
- AArch64 semantic terminal and next-operand consumers read the BIR-owned route.
- Tests are retargeted rather than weakened.
- No new schema behavior, consumer migration, or semantic broadening is bundled
  into this idea.
- Final proof covers oracle equivalence, focused AArch64 return-chain lowering,
  touched prepared lookup API tests, and any broader backend validation required
  by public surface changes.
