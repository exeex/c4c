# Ordered Or Exclusive Stack-Destination Fan-In Authority Runbook

Status: Active
Source Idea: ideas/open/647_ordered_or_exclusive_stack_destination_fan_in_authority.md

## Purpose

Resume stack-destination register fan-in authority work after idea 675 closed
and after decomposition idea 655 parked the coarse Step 2 route.

## Goal

Identify one legal non-637 prepared/prealloc destination-authority family for
stack-destination register fan-in, or record precise producer evidence proving
no implementation packet is available yet.

## Core Rule

Do not claim progress through named-case fixes, expectation rewrites,
unsupported-marker changes, allowlist edits, timeout/accounting changes, or
RV64 materialization before prepared/prealloc publishes explicit destination
authority facts.

## Read First

- `ideas/open/647_ordered_or_exclusive_stack_destination_fan_in_authority.md`
- `ideas/open/655_stack_destination_fan_in_authority_decomposition.md`
- `docs/destination_fan_in_authority/03_implementation_split.md`
- `ideas/closed/637_prepared_stack_destination_fan_in_authority_producer.md`

## Current Targets

- Residual non-637 stack-destination register fan-in rows named in idea 647.
- Parked decomposition evidence from idea 655.
- Focused prepared/prealloc producer facts for exactly one legal authority
  family, if current evidence proves one.

## Non-Goals

- Do not reopen idea 637's select-materialized semantic-merge contract.
- Do not implement RV64 target materialization before the producer authority
  fact exists.
- Do not infer authority from filenames, source order, value ids, block labels,
  diagnostics, move-vector order, source freshness, final assembly, ABI,
  runtime behavior, expectations, allowlists, timeouts, or unsupported markers.
- Do not treat decomposition-only notes as implementation progress.

## Working Model

Idea 637 closed one authority family:
select-materialized semantic merge with preserved stack fallback. Idea 647 owns
the remaining non-637 residual rows, but idea 655 records that the last broad
route did not prove a legal positive family. Execution must therefore refresh
the residual evidence, reconcile it with 655's focused seams, and choose only a
single family when producer facts are visible at the consumer program point.

## Execution Rules

- Compare rows by stable test name and underlying authority contract, not by
  numeric row id.
- Keep missing, unsupported, ambiguous, stale, and bundle-versus-move
  mismatched authority fail-closed.
- Prefer focused prepared/prealloc probes before changing shared producer
  logic.
- If the only positive evidence re-enters idea 637's closed contract, stop and
  route the result as blocked instead of implementing under 647.
- Preserve proof summaries under `build/agent_state/647_*` if new diagnostic
  artifacts are created.

## Steps

### Step 1: Refresh The Non-637 Residual Baseline

Goal: Establish the current failing residual set and reconcile it with parked
decomposition evidence.

Actions:

- Read idea 647, idea 655, and the destination fan-in implementation split.
- Refresh diagnostics for the residual rows named in idea 647.
- Classify each residual as ordered final-state, mutual-exclusion, explicit
  merge, rejection-only, stale, or reopened-637 evidence.
- Record any new evidence by stable test name and producer fact shape.

Completion Check:

- The current residual set is classified without selecting an implementation
  family by assumption.

### Step 2: Select One Producer Authority Family Or Park

Goal: Decide whether one non-637 authority family has enough positive producer
evidence for implementation.

Actions:

- Compare the refreshed residuals against idea 655's seams: ordered
  final-state, mutual-exclusion, explicit merge, and rejection authority.
- Choose exactly one family only if the producer can publish an explicit fact
  at the consumer program point.
- Preserve precise negative evidence for unsupported, missing, stale,
  ambiguous, and mismatched shapes.
- If no legal positive family exists, document the missing producer evidence
  and leave implementation blocked.

Completion Check:

- The route is explicit: one selected implementation family with proof targets,
  or a blocked/parked outcome with the missing evidence named.

### Step 3: Implement The Selected Family In Focused Packets

Goal: Add the smallest prepared/prealloc producer support for the selected
family, if Step 2 proves one is legal.

Actions:

- Add or update focused backend/prepared tests for one legal shape and one
  fail-closed negative shape.
- Publish producer facts only for the selected family and only at the proven
  consumer program point.
- Keep residuals outside the selected family fail-closed with durable notes.
- Run `cmake --build --preset default` and a focused backend CTest subset for
  the changed surface.

Completion Check:

- The selected family is represented in prepared/prealloc facts with explicit
  owner labels and negative states, and the focused proof is green.

### Step 4: Broader Validation And Residual Routing

Goal: Confirm the slice does not regress backend coverage and route remaining
residuals.

Actions:

- Run the supervisor-selected broader backend validation after focused proof.
- Compare remaining residuals against the selected authority family.
- Move unrelated or still-blocked residuals to a separate open idea only when
  a distinct initiative is proven.

Completion Check:

- Backend validation is recorded, remaining residual ownership is explicit,
  and idea 647 can either close or continue with a narrowed next packet.
