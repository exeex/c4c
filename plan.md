# Route 6 Call-Use Semantic Source Migration

Status: Active
Source Idea: ideas/open/172_route6_call_use_semantic_source_migration.md

## Purpose

Activate a narrow Route 6 migration that moves one call argument or result
semantic source consumer class onto BIR call-use source records while keeping
ABI and layout records prepared-owned.

## Goal

Migrate one selected call source consumer to `Route6CallUseSourceIndex` for
semantic source facts, prove the matching oracle coverage, and avoid broad call
lowering rewrites.

## Core Rule

Use Route 6 only for semantic call-use source facts. ABI register/stack
placement, layout, outgoing stack sizing, variadic FPR count, clobbers, byval
lanes, scratch requirements, destination homes, helper/carrier protocols, and
aggregate transport layout must remain prepared-call-plan owned.

## Read First

- ideas/open/172_route6_call_use_semantic_source_migration.md
- docs/bir_prealloc_fusion/phase_c_private_cache_contraction.md

## Current Scope

- Select exactly one call argument or result semantic source consumer class.
- Move that consumer's semantic source reads to `Route6CallUseSourceIndex`.
- Keep prepared call plans as the ABI/layout oracle and negative-control
  surface during the migration.
- Add or update oracle tests for source-producer, direct-global, eligible
  memory or publication, result/source facts, and ABI-bound negative cases for
  the selected class.

## Non-Goals

- Do not migrate multiple call source consumer classes in one packet unless the
  supervisor explicitly expands scope.
- Do not move ABI placement, aggregate transport, helper/carrier, byval,
  scratch, clobber, destination-home, or variadic policy into BIR.
- Do not contract broad prepared call plan APIs before the selected consumer has
  Route 6 proof.
- Do not weaken call-boundary tests or expectation contracts to claim cache
  contraction.

## Working Model

- `Route6CallUseSourceIndex` owns semantic source facts for migrated call-use
  consumers.
- Prepared call plans continue to own ABI and layout-bound facts.
- Oracle coverage must show both the positive semantic-source answers and the
  negative ABI-bound cases that should still come from prepared call plans.

## Execution Rules

- Work in small packets that each name the selected consumer boundary.
- Prefer route-index validation and semantic lowering over testcase-shaped
  matching.
- Keep source idea edits unnecessary unless durable intent changes.
- For code-changing packets, run build or compile proof plus the delegated
  narrow test subset. Escalate to broader backend checks if public call APIs or
  plan projections change.
- Treat helper renames, expectation rewrites, or classification-only changes as
  insufficient progress unless they enable the semantic Route 6 migration and
  are proven by behavior.

## Ordered Steps

### Step 1: Select The Consumer Boundary

Goal: identify one call argument or result semantic source consumer class that
can migrate without pulling ABI/layout policy into BIR.

Primary targets:

- Existing call argument/result source lookup helpers and their consumers.
- Existing Route 6 call-use source records and validation helpers.
- Existing call-source oracle tests.

Actions:

- Inventory candidate consumers that read source-producer, direct-global,
  eligible memory/publication, result, or lane semantic source facts.
- Pick one consumer class with a narrow proof route and clear ABI-bound
  negative cases.
- Record the selected consumer and proof command in `todo.md` before code
  edits begin.

Completion check:

- `todo.md` names the selected consumer class, its Route 6 semantic-source
  facts, and the narrow test subset the executor should use.

### Step 2: Add Route 6 Oracle Coverage

Goal: prove Route 6 can answer the selected semantic source facts before
contracting the consumer.

Primary targets:

- Route 6 oracle tests for the selected consumer class.
- Any existing call-source oracle harness for semantic source records.

Actions:

- Add positive cases for source-producer, direct-global, eligible memory or
  publication, and result/source facts relevant to the selected class.
- Add ABI-bound negative cases that demonstrate prepared call plans remain the
  owner for layout and placement facts.
- Keep test expectations focused on semantic source answers, not final call
  lowering shape.

Completion check:

- The selected call-source oracle tests fail for missing Route 6 answers before
  implementation or pass with an existing implementation, and they do not
  weaken existing call-boundary expectations.

### Step 3: Migrate The Selected Consumer

Goal: route the selected consumer's semantic source reads through
`Route6CallUseSourceIndex`.

Primary targets:

- The selected consumer's semantic source lookup path.
- Route 6 source index accessors or validation helpers.
- Prepared call plan reads used by the same consumer.

Actions:

- Replace only semantic source reads with Route 6 lookup/validation.
- Leave ABI/layout-bound reads on prepared call plans.
- Keep fallback behavior fail-closed when Route 6 lacks the required typed
  source record.
- Avoid copying prepared call plan payloads into BIR records.

Completion check:

- The selected consumer uses `Route6CallUseSourceIndex` for semantic source
  facts, and ABI/layout facts still read from prepared call plans.

### Step 4: Prove The Narrow Migration

Goal: validate the selected consumer migration without treating a narrow green
case as broad call lowering completion.

Primary targets:

- The selected call-source oracle subset.
- The narrow call-lowering subset that exercises the selected consumer.

Actions:

- Run the delegated build or compile proof.
- Run call-source oracle tests for the selected class.
- Run the narrow call-lowering subset for that class.
- Escalate to broader backend tests if public call APIs or plan projections
  changed.

Completion check:

- Fresh proof shows the selected Route 6 oracle coverage and narrow
  call-lowering subset pass, with no expectation downgrades.

### Step 5: Contract Only Proven Prepared Surface

Goal: remove or narrow prepared semantic-source cache surface only where the
selected consumer no longer requires it.

Primary targets:

- Prepared call semantic-source helpers used only by the migrated consumer.
- Call-source cache or oracle projection code made redundant by Step 3.

Actions:

- Contract only the prepared semantic-source surface proven redundant by the
  selected migration.
- Keep prepared ABI/layout helpers and oracle surfaces intact.
- Do not generalize contraction beyond the selected consumer class.

Completion check:

- Redundant prepared semantic-source surface for the selected consumer is
  removed or narrowed, ABI/layout surfaces remain prepared-owned, and the Step 4
  proof still passes.
