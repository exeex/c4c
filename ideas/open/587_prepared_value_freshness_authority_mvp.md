# Prepared Value Freshness Authority MVP

Status: Open
Type: Architecture implementation MVP
Parent: `ideas/closed/585_target_abi_contract_and_value_consumption_research.md`
Related: `ideas/open/586_uniform_target_register_identity_policy.md`
Owning Layer: Prepared/prealloc value-consumption authority, producer
rematerialization precedence, preservation freshness, and target consumer
handoff

## Goal

Introduce a minimal first-class prepared value freshness authority model and
wire it into the highest-risk value consumers so the compiler can answer, for
a concrete use, which source is fresh enough to trust: a direct home, an
explicit publication, a producer rematerialization, a prior-preserved home, or
a move-bundle source.

This is an MVP, not the final global dataflow design. It should make the
current distributed decision model observable, queryable, and fail-closed for
the first representative consumer families.

## Why This Exists

The target ABI contract research in
`docs/target_abi_contract_research/` concluded that the pipeline direction is
healthy, but value freshness is not a first-class contract. Today freshness is
inferred locally from value homes, publication rows, producer facts,
`PriorPreservation`, move bundles, call plans, and target-specific backend
checks.

That distributed model has worked for narrow fixes, but recent closed ideas
show the same architectural symptom recurring:

- a prepared home can be internally coherent but stale relative to a later
  producer or missing publication;
- a producer rematerialization can be the correct source for a call argument
  even when an older preserved home exists;
- move-bundle destination authority does not necessarily prove source
  freshness;
- RV64 and AArch64 consumers perform local completeness checks instead of
  consulting one shared freshness authority.

The MVP should define and publish explicit freshness facts for selected uses,
then make target consumers consult those facts before falling back to the old
distributed ordering.

## Working Model

The implementation should add a small authority surface equivalent to:

```text
PreparedValueFreshnessAuthority
  value_id / value name
  use kind
  use program point
  source kind
  proof kind
  source storage or producer/publication/preservation/move reference
  rank or precedence class
```

The exact names and storage layout should follow the existing prepared/prealloc
style, but the model must distinguish these concepts:

- value identity: which prepared value the authority is for
- use context: call argument, move-bundle source, select/publication operand,
  store value, or another explicitly supported use
- source kind: direct home, producer rematerialization, explicit publication,
  prior preservation, move-bundle source, ABI/formal home
- proof kind: same-block-before-use, dominance/ordering, explicit
  publication, ABI entry/formal home, call-boundary preservation,
  move-bundle authority, or fail-closed unknown
- precedence: which source wins when more than one source exists for the same
  value/use

## In Scope

- Add a prepared/prealloc freshness authority representation and lookup/query
  helper for the MVP-supported use kinds.
- Publish freshness authority facts from existing prepared/prealloc facts
  without inventing new semantic producers:
  - direct prepared value homes when they are valid for the requested use;
  - explicit producer-rematerialization facts already available to call
    argument or publication paths;
  - explicit publication rows where the publication point proves current
    value availability;
  - `PriorPreservation` records only when the preserved route is unique,
    complete, dominance/position-valid, and target-verifiable;
  - move-bundle sources only when the bundle has visible source authority for
    the use.
- Define MVP precedence rules, with producer rematerialization and explicit
  publication outranking older preserved homes when both are valid for the
  same use.
- Wire the freshness query into the first representative consumers:
  - prepared call argument source selection, especially producer
    rematerialization versus `PriorPreservation`;
  - RV64 prepared/object call argument consumption where stale homes have
    previously appeared;
  - prepared move-bundle source consumption for stack-destination or
    publication-related moves where source authority is currently implicit.
- Add focused tests and/or prepared dump assertions showing the selected
  freshness authority for representative sources.
- Preserve existing fail-closed behavior when no freshness authority can be
  proven.
- Record unsupported or incomplete authority shapes with narrow diagnostics or
  verifier statuses instead of silently using an old home.

## Out Of Scope

- Replacing all prepared value-home, publication, preservation, and
  move-bundle APIs in one slice.
- Building a full global dataflow lattice for every value and every consumer.
- Changing target triple parsing, `TargetProfile`, or BIR semantic ABI
  classification.
- Implementing uniform target physical register identity publication; that is
  tracked separately by `ideas/open/586_uniform_target_register_identity_policy.md`.
- Reworking the stack-destination move-bundle authority taxonomy already
  repaired by idea 584, except to consume or link source freshness facts where
  the MVP-supported use requires it.
- Broad RV64, AArch64, or x86 backend rewrites.
- Expectation rewrites, unsupported-marker edits, allowlist changes, or
  runtime-comparison changes as proof of freshness progress.

## MVP Acceptance Criteria

- A prepared/prealloc freshness authority data model exists and is visible in
  code and tests or prepared dumps.
- The MVP supports at least these use kinds:
  - call argument source;
  - move-bundle source;
  - producer/publication operand for a supported prepared object or
    publication route.
- A shared query/helper returns the selected freshness source or a precise
  fail-closed status for a value/use pair.
- Focused tests prove that producer rematerialization or explicit publication
  outranks `PriorPreservation` when both are present and the producer or
  publication is valid for the use.
- Focused tests prove that `PriorPreservation` remains accepted when it is the
  unique complete dominance/position-valid source and no fresher producer or
  publication exists.
- Focused tests prove that a move-bundle source without sufficient source
  freshness authority fails closed instead of being accepted only because the
  destination bundle is well-formed.
- At least one RV64 consumer path consults the freshness authority before
  using a call argument or move-bundle source that previously depended on
  local fallback ordering.
- Any AArch64 impact is either wired through the shared helper or explicitly
  documented as not yet consuming the MVP authority, with existing AArch64
  fail-closed checks preserved.
- Existing focused backend/prealloc tests for call plans, value homes, move
  bundles, and RV64 object emission continue to pass.
- The closure note must include a concrete remaining-consumer inventory:
  - every consumer path wired to the MVP freshness query;
  - every nearby consumer path intentionally not wired;
  - the reason each unwired path is deferred, already protected, or out of
    scope;
  - the evidence or test that proves the unwired path is not silently relying
    on stale-home behavior;
  - recommended next ideas or discussion topics for any consumer that still
    needs freshness authority.

## Closure Note Requirements

When closing this idea, the closure note must answer:

1. Which freshness authority source kinds were implemented?
2. Which use kinds are supported by the MVP?
3. Which consumers now consult the shared freshness query?
4. Which RV64 consumers still use local ordering or local fallback checks?
5. Which AArch64 consumers still use local ordering or local fallback checks?
6. Which x86 or shared-prealloc consumers were reviewed but not wired?
7. Which existing fail-closed diagnostics remain the protection for unwired
   consumers?
8. Which stale-home / missing-producer / move-source cases are now covered by
   focused tests?
9. What concrete follow-up ideas should be opened next, if any?

Do not close this idea with a generic "future work remains" note. The closure
must name the remaining consumer families and state whether they are safe to
defer, blocked on design, or ready for a narrow follow-up implementation idea.

## Representative Evidence To Recheck

Use the following closed idea families as regression/evidence anchors while
choosing focused tests:

- `ideas/closed/576_rv64_pr56982_post_carrier_runtime_mismatch.md`: symbol
  address call arguments should rematerialize instead of trusting stale
  preserved homes.
- `ideas/closed/584_rv64_20000819_runtime_mismatch_after_pointer_publication.md`:
  computed global-address call arguments must materialize from semantic
  relocation rather than copying stale `s1`.
- `ideas/closed/583_rv64_pointer_arithmetic_result_publication.md`: pointer
  arithmetic materialization is not enough unless the result is published to
  the prepared home later consumers trust.
- `ideas/closed/462_rv64_preterminator_predecessor_edge_parallel_copy_materialization.md`:
  a value produced after a predecessor terminator is not fresh at that
  predecessor terminator just because a prepared home exists later.
- `ideas/closed/579_rv64_prepared_stack_destination_move_bundle_authority.md`
  and `ideas/closed/584_rv64_stack_destination_move_bundle_authority_contract.md`:
  destination fan-in authority does not automatically prove source freshness.

## Reviewer Reject Signals

- Reject a slice that only renames existing value-home, publication, or
  preservation helpers without introducing a queryable freshness authority.
- Reject backend-local ordering changes that make one RV64 case pass without
  publishing or consulting shared prepared/prealloc freshness facts.
- Reject treating `PriorPreservation` as fresh solely because the storage
  payload is complete when a valid same-use producer rematerialization or
  explicit publication exists.
- Reject accepting a move-bundle source based only on destination authority
  when the source freshness proof is missing or ambiguous.
- Reject broad target ABI, register identity, or call-lowering rewrites under
  this MVP unless they are strictly necessary to expose the freshness query.
- Reject testcase-shaped matching for named torture files, exact function
  names, exact register names, or exact diagnostic strings.
- Reject expectation rewrites, unsupported-marker changes, allowlist edits, or
  runtime-output changes as freshness authority progress.
- Reject a route that removes existing fail-closed diagnostics before the MVP
  freshness verifier covers the same malformed or unknown-authority shapes.
