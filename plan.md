# Ordered Or Exclusive Stack-Destination Fan-In Authority Runbook

Status: Active
Source Idea: ideas/open/647_ordered_or_exclusive_stack_destination_fan_in_authority.md

## Purpose

Define the next prepared/prealloc destination authority family for
stack-destination register fan-in residuals that are outside the
select-materialized semantic-merge contract closed by idea 637.

## Goal

Classify the remaining stack-destination fan-in residuals, choose one
ordered-final-state or mutual-exclusion producer authority family, and prove
that family through explicit prepared/prealloc facts with fail-closed negative
states.

## Core Rule

Do not infer stack-destination fan-in authority from testcase identity, source
order, move-vector order, diagnostics, source freshness, final assembly, ABI
behavior, or expectation changes. Progress requires semantic producer
authority facts at the consumer program point.

## Read First

- `ideas/open/647_ordered_or_exclusive_stack_destination_fan_in_authority.md`
- `docs/destination_fan_in_authority/03_implementation_split.md`
- Prior related context when needed:
  - `ideas/closed/607_destination_fan_in_authority_research.md`
  - `ideas/closed/637_prepared_stack_destination_fan_in_authority_producer.md`

## Current Targets

- `src/20011109-2.c`
- `src/20021204-1.c`
- `src/920429-1.c`
- `src/930429-1.c`
- `src/pr34415.c`
- `src/ptr-arith-1.c`
- `src/pr70005.c`

These rows are candidates only. The route must classify them first and then
select exactly one first producer family before implementation.

## Non-Goals

- Do not reopen idea 637's select-materialized semantic-merge contract.
- Do not implement RV64 target materialization for a newly authorized family
  unless a downstream consumer idea is separately activated.
- Do not broaden string-constant local-memory policy.
- Do not rewrite expectations, unsupported markers, allowlists, timeouts, or
  pass/fail accounting as capability progress.
- Do not add named-case shortcuts for the listed residuals.

## Working Model

Idea 637 proved one destination-authority family:
select-materialized semantic merge with preserved stack fallback. The current
residuals remain fail-closed because their fan-in shapes require a different
authority family, likely ordered final-state authority, mutual-exclusion
authority, or another explicitly named prepared/prealloc destination authority.

The route should produce facts only when the producer can prove the selected
family for a stack destination at the consumer point. Unsupported, missing,
ambiguous, stale, bundle-versus-move mismatched, and unrelated fan-in shapes
must keep precise fail-closed diagnostics.

## Execution Rules

- Keep routine investigation notes and packet progress in `todo.md`.
- Prefer focused probes and narrow tests before touching shared producer code.
- Select only one first producer family for implementation under this plan.
- If refreshed evidence proves a separate initiative is needed, record that as
  lifecycle state instead of silently expanding this plan.
- For each code-changing step, run a fresh build proof and the focused backend
  or prepared/prealloc subset selected by the supervisor.
- Escalate to broader backend validation before accepting a capability slice
  if producer changes affect shared prepared/prealloc authority behavior.

## Steps

### Step 1: Refresh Residual Evidence

Goal: Reproduce the current first-owner diagnostics for the residual rows and
capture enough prepared/prealloc detail to classify their fan-in shapes.

Primary targets:

- `src/20011109-2.c`
- `src/20021204-1.c`
- `src/920429-1.c`
- `src/930429-1.c`
- `src/pr34415.c`
- `src/ptr-arith-1.c`
- `src/pr70005.c`

Actions:

- Run focused diagnostics for each target through the existing backend or
  prepared/prealloc route used for destination fan-in authority evidence.
- Record the block, destination, source values, move bundle classification,
  predecessor context, and current authority or fragment status.
- Distinguish `missing_stack_destination_fan_in_authority_fact`,
  `producer_authority_missing_for_register_fan_in_stack_destination`, and
  `unsupported_prepared_move_bundle_classification` rows.
- Store durable evidence under `build/agent_state/` rather than root-level
  ad hoc logs.

Completion check:

- Each target row has refreshed evidence and a concise classification note in
  `todo.md` naming the observed owner and fan-in shape.

### Step 2: Revise Producer Authority Family Selection

Goal: Select exactly one first destination authority family that is supported
by refreshed evidence after rejecting the attempted mutual-exclusion packet for
`src/20021204-1.c`.

Actions:

- Reclassify candidate rows as ordered final-state authority,
  mutual-exclusion authority, merge authority, or another explicitly named
  family.
- Treat `src/20021204-1.c` at `main:tern.end.12` before instruction 1 as
  rejected for the attempted mutual-exclusion family: the failing
  `%t20/%t21 -> %t22` stack-destination bundle has no acceptable predicate,
  edge, selected-active-candidate, guarded-copy, or control-flow carrier at the
  consumer point.
- Do not reuse the unrelated `%t17/%t24 -> %t25` select edge/control-flow facts
  as authority for `%t20/%t21 -> %t22`.
- Choose one revised family for implementation only after naming the minimal
  positive and negative examples. Ordered final-state authority is the likely
  next candidate, but it still requires evidence that the producer designates a
  final authoritative stack-slot state at the consumer point.
- Leave other families fail-closed with durable notes in `todo.md`; create a
  separate open idea only if the residual family is distinct and ready for
  lifecycle tracking.
- Identify the prepared/prealloc fact shape, owner label, negative statuses,
  and consumer program point required for the selected family.

Completion check:

- `todo.md` records the rejected `src/20021204-1.c` mutual-exclusion route,
  names the revised selected family, the first target row, the fact shape to
  publish, and the residual rows intentionally left out of scope.

### Step 3: Publish Revised Selected Producer Authority

Goal: Add prepared/prealloc producer support for the selected family without
guessing from incidental row shape.

Actions:

- Implement authority publication only where the revised selected semantic
  family is proven by Step 2 evidence.
- Do not implement the rejected `src/20021204-1.c` mutual-exclusion route unless
  a later Step 2 revision finds new producer proof that is not source
  availability, same-block order, arithmetic operand shape, value-id shape,
  diagnostics, testcase identity, final assembly, or the unrelated `%t25`
  select facts.
- Preserve precise fail-closed states for missing authority, ambiguous
  authority, stale source or destination facts, and move-bundle versus
  individual-move mismatches.
- Avoid RV64 consumption or materialization changes unless they are strictly
  required to observe the prepared/prealloc fact and remain inside the source
  idea boundary.

Completion check:

- A focused positive case publishes the selected authority fact, and focused
  negative evidence still rejects unsupported or mismatched fan-in shapes.

### Step 4: Add Focused Coverage

Goal: Lock the selected authority family and its rejection behavior with
targeted tests.

Actions:

- Add or update focused prepared/prealloc or backend assertions for one legal
  selected-family shape.
- Add or update at least one missing-authority or mismatched-authority
  rejection assertion.
- Keep residual rows outside the selected family fail-closed unless explicitly
  split into a new idea.

Completion check:

- Focused tests prove both selected-family publication and negative rejection
  without expectation weakening.

### Step 5: Validate And Classify Leftovers

Goal: Prove the implemented authority family and record the disposition of
residuals outside the selected family.

Actions:

- Run the fresh build proof and the focused test subset selected for this
  plan.
- Run broader backend validation if producer changes touch shared
  prepared/prealloc authority paths.
- Refresh target residuals enough to confirm that selected-family rows
  advanced and unrelated rows remain precise, fail-closed diagnostics.
- Record any separate residual family as a follow-up lifecycle candidate
  rather than expanding this plan.

Completion check:

- The selected authority family has green focused proof, no testcase-overfit
  signs, and `todo.md` records accepted leftovers or follow-up idea needs.
