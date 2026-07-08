# Select-Carrier Alias Freshness Contract

Status: Open
Type: Architecture contract and narrow consumer migration
Parent: `ideas/open/595_prepared_value_architecture_followup_umbrella.md`
Related:
- `ideas/closed/587_prepared_value_freshness_authority_mvp.md`
- `ideas/closed/588_shared_prealloc_move_operand_source_freshness_inventory.md`
- `ideas/closed/589_direct_edge_publication_move_freshness_ownership.md`
Owning Layer: shared-prealloc consumer authority for select-carrier alias
source acceptance

## Goal

Define the freshness-vs-alias ownership rule for select-carrier alias source
acceptance, then migrate at most one representative shared-prealloc consumer
only after the rule is explicit.

The key question is whether an alias fact can ever be promoted to selected
source freshness for a specific select-carrier use, or whether alias facts are
only support facts that must remain insufficient without a distinct freshness
authority.

## Why This Exists

Idea 587 introduced the prepared value freshness authority MVP. Idea 588
inventoried shared-prealloc move and operand source consumers and left
select-carrier/select-alias authority blocked on contract design. Idea 589
closed direct edge-publication source freshness and explicitly rejected
destination bundle legality, complete source homes, direct homes, alias
evidence, and local move shape as source freshness for that route.

The remaining select-carrier alias question is narrow enough to split from
destination fan-in, predecessor-edge consumed suppression, target backend
migration, and Prepared MIR view design. It needs one shared-prealloc
consumer authority rule before implementation work should proceed.

## Prerequisites

- Closed idea 587 freshness authority vocabulary and fail-closed lookup model.
- Closed idea 588 shared-prealloc inventory, especially the select-alias
  blocked-on-contract note.
- Closed idea 589 direct edge-publication ownership rule, especially the
  boundary that alias or destination facts are not source freshness by
  themselves.

## In Scope

- Audit select-carrier and select-alias shared-prealloc consumer surfaces that
  currently accept, reject, or record alias-shaped source evidence.
- State the selected ownership rule for select-carrier alias source
  acceptance:
  - which use kind is queried;
  - which source kinds, if any, are accepted;
  - whether a distinct freshness use/source kind is required;
  - which alias-only, destination-only, suppression-only, or structural facts
    are explicitly insufficient.
- If the contract is concrete, migrate one representative select-carrier alias
  consumer to require selected source freshness before accepting the source.
- Preserve or add fail-closed status for missing, ambiguous, stale,
  wrong-value, wrong-use, alias-only, destination-only, and structurally
  complete but freshness-less routes.
- Add focused tests or prepared dump assertions proving that the migrated
  route accepts explicit selected source freshness and rejects alias-only
  authority.

## Out Of Scope

- Destination fan-in authority.
- Predecessor-edge consumed suppression.
- Broad move-bundle scheduler rewrites or parallel-copy legality redesign.
- Shared-prealloc producer-publication repair outside the selected
  select-carrier alias route.
- RV64, AArch64, x86, string assembly, or other target consumer migration.
- Prepared MIR view design or `PreparedBirModule` interface slimming.
- Expectation rewrites, unsupported-marker edits, allowlist changes, runtime
  behavior changes, or harness changes.

## Acceptance Criteria

- The implementation or research packet identifies the audited
  select-carrier alias consumer set.
- The packet states a concrete freshness ownership rule for select-carrier
  alias source acceptance.
- If a consumer is migrated, it consults selected shared freshness authority
  before accepting the source.
- Missing, ambiguous, stale, wrong-value, wrong-use, alias-only,
  destination-only, and structurally complete but freshness-less authority
  fail closed with precise status or diagnostics.
- Focused tests or prepared dumps prove the accepted route is authorized by
  explicit selected source freshness rather than alias metadata, destination
  legality, suppression facts, complete homes, or target-local operand shape.
- Existing 587, 588, and 589 freshness authority tests continue to pass.
- Any need for destination fan-in, predecessor-edge suppression, producer
  publication, target migration, or MIR view work is recorded as a separate
  follow-up instead of being hidden in this slice.

## Reviewer Reject Signals

- Reject accepting a source because alias metadata, destination legality,
  suppression state, complete homes, or target operand shape exists while
  selected source freshness is absent.
- Reject testcase-shaped matching for named functions, exact source files,
  exact select shapes, exact diagnostic strings, or exact target registers.
- Reject expectation downgrades, unsupported-marker edits, allowlist edits,
  runtime-output changes, or diagnostic-only changes as proof of progress.
- Reject overloading `MoveBundleSource`, `DirectEdgePublicationSource`, or any
  unrelated freshness kind when the selected route needs a distinct ownership
  boundary.
- Reject broad mixed ownership that combines select alias, destination fan-in,
  predecessor-edge suppression, shared producer publication, target consumer
  migration, and Prepared MIR view design.
- Reject retaining the same alias-only or destination-only source freshness
  failure mode behind a renamed helper or abstraction.

## Closure Note Requirements

The closure note must answer:

1. Which select-carrier alias consumers were audited?
2. What is the freshness ownership rule for select-carrier alias source
   acceptance?
3. Which freshness use kinds and source kinds were used or added?
4. Which representative consumer, if any, was migrated?
5. Which missing, ambiguous, stale, wrong-value, wrong-use, alias-only, and
   destination-only cases fail closed?
6. Which adjacent families were intentionally left out, especially
   destination fan-in and predecessor-edge consumed suppression?
7. Which tests or prepared dumps prove explicit selected source freshness
   rather than alias or destination authority?
8. What concrete follow-up ideas should be opened next, if any?
