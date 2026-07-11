# Residual Stack Authority Revisit Prerequisites

Status: Closed
Type: Decomposition
Parent: `ideas/open/694_bir_route_index_retirement_umbrella.md`
Related:
- `ideas/open/647_ordered_or_exclusive_stack_destination_fan_in_authority.md`
- `ideas/open/655_stack_destination_fan_in_authority_decomposition.md`
Handoff:
- `docs/bir_route_index_retirement/research_digest.md`
- `docs/bir_route_index_retirement/ownership_dependencies.md`
- `docs/bir_route_index_retirement/ordered_followup_plan.md`
Queue Order: 8
Depends On:
- `ideas/closed/700_prepared_mir_stack_view_contract.md`

## Goal

Define the positive prepared producer evidence required before ideas 647 or
655 can resume residual stack destination authority work.

## Why This Exists

The route-retirement handoff explicitly parks ideas 647 and 655. Route 4,
Route 5, Route 7, route-index facade status, dump rows, expectations, and
allowlists can show historical compatibility only. They cannot choose a stack
destination, authorize a move bundle, select freshness, or make MIR lower a
stack path.

## Owned Files

- `ideas/open/647_ordered_or_exclusive_stack_destination_fan_in_authority.md`
  and `ideas/open/655_stack_destination_fan_in_authority_decomposition.md`
  only if a future lifecycle packet needs to record new durable prerequisite
  evidence.
- Prepared/prealloc probe or documentation files only in a later activated
  plan that proves a positive producer seam.

## First Owning Layer

Prepared stack authority producer evidence.

## First Producer Migration

No implementation migration is allowed in this idea until a positive prepared
producer seam is identified above route dumps. The first acceptable migration
must name destination value identity, destination home, destination storage
kind, source value/home, selected move bundle or move resolution, selected
freshness, stack object or aggregate source authority, and MIR fail-closed
statuses as applicable.

## Proof Surface

Positive producer proof above route dumps for selected value homes, move
resolution, freshness, aggregate stack source, branch stack-load, and MIR
fail-closed behavior. Prepared contract, prepared MIR, object,
object-runtime, or runtime proof is required when executable behavior changes.

## Numbered Route APIs Kept Private Compatibility

- Route 4 publication rows
- Route 5 status and agreement rows
- Route 7 comparison validation rows
- `RouteIndexReferenceFacade`
- route dump rows and expected output labels

These are insufficient revisit evidence. They may be cited only as historical
compatibility or as inputs to named prepared producers through named BIR views.

## In Scope

- Define the minimum proof threshold for reactivating idea 647 or 655.
- Reject route-only evidence as a residual stack authority prerequisite.
- Identify whether a future positive seam belongs to ordered final-state,
  mutual-exclusion, explicit merge, aggregate stack-source, branch stack-load,
  or another named prepared producer family.
- Keep 647 and 655 parked until the threshold is met.

## Out Of Scope

- Implementing stack destination authority.
- Reactivating 647 or 655 from route agreement or dump changes.
- RV64 target materialization before prepared producer evidence exists.
- Test expectation, unsupported marker, allowlist, or timeout changes.

## Acceptance Criteria

- The first future activation condition for ideas 647 and 655 is explicit and
  prepared-owned.
- Route-numbered compatibility evidence is rejected as direct stack authority.
- Any selected follow-up producer family names its first positive and negative
  proof surfaces before implementation starts.
- Ideas 647 and 655 remain parked unless the prepared producer threshold is
  met.

## Completion Summary

Closed after todo-only prerequisite classification. The activation threshold is
prepared-owned positive producer evidence above route dumps, with matching
negative fail-closed proof, for exactly one named future producer family before
ideas 647 or 655 can resume.

No positive residual stack-destination fan-in seam was proven in this runbook.
Ideas 647 and 655 remain parked. Route-only evidence remains rejected as direct
stack authority, including Route 4 publication rows, Route 5 status or
agreement rows, Route 7 comparison validation rows, `RouteIndexReferenceFacade`,
route dumps, dump labels, expectations, allowlists, diagnostics, final assembly,
ABI/runtime behavior, testcase identity, source order, value ids, block labels,
move-vector order, frame-slot existence, source freshness alone, and
string-label pointer authority.

Future lifecycle work may revisit ideas 647 or 655 only after a focused
positive producer proof and matching negative fail-closed proof satisfy the
threshold for ordered final-state authority, mutual-exclusion authority,
explicit merge authority, aggregate stack-source authority, or another
explicitly named non-637 destination-authority producer family.

## Reviewer Reject Signals

- Reject using Route 4, Route 5, Route 7, facade status, route dumps,
  expectations, or allowlists as stack destination authority.
- Reject reactivating ideas 647 or 655 without positive prepared producer
  evidence above route dumps.
- Reject named-case shortcuts around the historical residual GCC torture rows.
- Reject RV64 materialization before a prepared/prealloc authority fact is
  visible on the participating value homes, bundles, moves, freshness, and
  destination records.
- Reject classification-only edits or diagnostic wording changes claimed as
  authority progress.
