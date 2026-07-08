# Select-Carrier Alias Freshness Contract

Status: Closed
Type: Architecture contract and narrow consumer migration
Parent: `ideas/closed/595_prepared_value_architecture_followup_umbrella.md`
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

## Closure Notes

Closed after the active runbook completed all five steps and the Step 5
handoff answered every closure-note requirement.

Audited consumers and surfaces:
`PreparedSelectCarrierAliasAuthority`,
`PreparedSelectCarrierAliasAuthorityRecords`,
`plan_prepared_select_carrier_alias_authority(...)`,
`collect_prepared_select_carrier_alias_authorities(...)`,
`collect_prepared_select_carrier_alias_authority_evidence(...)`,
`populate_select_carrier_alias_identity(...)`,
`append_select_carrier_alias_authorities(...)`,
`classify_prepared_object_select_consumer(...)`,
`diagnose_prepared_object_consumer(...)`, RV64 admission threading through
`RiscvPreparedFunctionAdmissionResult`, RV64 helper
`prepared_select_edge_binary_source_has_carrier_alias_authority(...)`, RV64
carrier-alias select suppression through
`prepared_select_is_authorized_carrier_alias(...)`, and the RV64 object
emission call sites that pass carrier-alias authority records.

Ownership rule:
the binary select-edge publication source is fresh for the representative
select-carrier alias use only when a selected shared freshness authority
matches the exact function, predecessor, successor, destination value/id/name,
source value/id/name, binary source producer kind, source producer block/inst,
proof, source kind, rank, and alias closure required by the selected
publication/source use. Alias metadata, destination legality, suppression,
complete homes, target shape, and structural join-transfer evidence are support
facts only.

Freshness vocabulary:
added `PreparedValueFreshnessUseKind::SelectCarrierAliasSource`,
`PreparedValueFreshnessSourceKind::SelectCarrierAlias`,
`PreparedValueFreshnessProofKind::SelectCarrierAliasAuthority`, and
`PreparedValueFreshnessSourceRank::SelectCarrierAlias`. The route did not
reuse `MoveBundleSource` or `DirectEdgePublicationSource`.

Migrated representative consumer:
the source acceptance gate reached through RV64
`prepared_select_edge_binary_source_has_carrier_alias_authority(...)` inside
`prepared_select_edge_binary_source_has_authorized_consumers(...)` now
delegates to shared-prealloc
`prepared_select_carrier_alias_source_freshness_available(...)`.

Fail-closed behavior:
missing/no-candidate freshness, ambiguous freshness, stale or wrong program
point/reference, wrong value, wrong use, wrong source kind, wrong proof, wrong
rank, and alias-only authority fail closed before source acceptance.
Destination-only authority remains insufficient by contract; no destination
home, destination register, stack-destination fan-in, or destination-bundle
fact is used as select-carrier alias source freshness. Existing unavailable
carrier-alias statuses such as missing source producer, mismatched alias, and
non-carrier source use remain fail closed.

Intentionally left out:
destination fan-in authority, predecessor-edge consumed suppression,
producer-publication repair outside this route, broad RV64/AArch64/x86/string
target migration, move-bundle scheduler or parallel-copy redesign,
pointer/address work, Prepared MIR view design, and dump-output expansion for
selected carrier-alias freshness fields.

Proof surfaces:
`check_select_carrier_alias_authority_contract()` asserts selected
`SelectCarrierAliasSource` freshness on accepted authority records and direct
fail-closed behavior for alias-only, ambiguous, stale/wrong-reference,
wrong-value, wrong-use, wrong-source, wrong-proof, and wrong-rank cases.
Existing prepared dump assertions prove carrier-alias evidence rows expose
available and rejected alias authority status, candidate counts, aliases, and
source-use closure; selected freshness fields are proven by direct contract
tests rather than dump text.

Follow-up decision:
no required follow-up is needed to complete this source idea. Optional future
ideas, if prioritized separately, are prepared dump output for selected
carrier-alias freshness fields, destination fan-in authority,
predecessor-edge consumed suppression, broader target consumer migration, and
Prepared MIR view/interface cleanup.

Close validation:
the plan-owner close gate used existing matching canonical backend logs and
ran
`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`;
the guard passed with 346/346 before and 346/346 after.
