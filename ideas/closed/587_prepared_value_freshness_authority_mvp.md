# Prepared Value Freshness Authority MVP

Status: Closed
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

## Closure Note

Closed on 2026-07-08 after Step 7 acceptance inventory and backend regression
guard.

Implemented freshness authority source kinds:

- `DirectHome` for call arguments and move-bundle source authority when a
  complete source-side prepared home is visible.
- `ProducerRematerialization` for call arguments and the supported
  store-source publication operand route.
- `ExplicitPublication` for call-argument publication moves.
- `PriorPreservation` for unique complete call-boundary preservation.
- `MoveBundleSource` as a vocabulary/source-kind entry; the accepted
  move-bundle consumer rejects candidates whose only proof is the consumed
  move bundle itself.

Supported MVP use kinds:

- `CallArgumentSource`
- `MoveBundleSource`
- `ProducerPublicationOperand`

Consumers now consulting the shared freshness query:

- Shared prepared call-plan publication in
  `src/backend/prealloc/call_plans.cpp` publishes `CallArgumentSource`
  candidates for direct homes, explicit publications, producer
  rematerialization, and unique complete prior preservation.
- RV64 prepared call emission in
  `src/backend/mir/riscv/codegen/prepared_call_emit.cpp` requires selected
  freshness for stale-home-prone call-argument sources, including
  prior-preserved GPR arguments and register-sourced arguments with published
  candidates.
- Shared prepared object move-bundle classification in
  `src/backend/prealloc/prepared_object_traversal.cpp`, used by RV64 object
  emission through `src/backend/mir/riscv/codegen/object_emission.cpp`, queries
  `MoveBundleSource` freshness before accepting a source.
- Shared store-source publication planning in
  `src/backend/prealloc/publication_plans.cpp` publishes and selects
  `ProducerPublicationOperand` freshness for validated same-block
  store-source producers.

RV64 consumers still using local ordering or local fallback checks:

- RV64 object-emission value-home helpers such as `prepared_value_home_for`,
  `gpr_register_number_for_value`, `fpr_register_number_for_value`, and
  rematerializable-immediate reads still query prepared homes directly. They
  are deferred because they are broad object-value consumers outside the MVP
  use kinds unless reached through the wired move-bundle classifier or
  prepared call emitter. Existing protection is local contract verification and
  prepared object consumer diagnostics for missing, ambiguous, unsupported, and
  incomplete homes, covered by
  `tests/backend/bir/backend_prepared_object_consumer_contract_test.cpp`.
- RV64 frame-slot address/value and byval aggregate call-argument paths in
  `src/backend/mir/riscv/codegen/prepared_call_emit.cpp` still rely on
  structured source-selection validators and stack/aggregate payload checks
  unless freshness candidates are published for a register source. They are
  deferred because stack-source and aggregate-lane freshness need a follow-up
  policy.

AArch64 consumers still using local ordering or local fallback checks:

- AArch64 call consumers in `src/backend/mir/aarch64/codegen/calls.cpp` still
  use `find_prepared_call_argument_publication_source_routing`,
  `make_selected_call_argument_source`, prior-preservation payload checks, and
  byval/register-lane completeness checks. They are intentionally deferred as
  a second target-consumer migration beyond the representative RV64 path.
- AArch64 prepared object traversal in
  `src/backend/mir/aarch64/codegen/traversal.cpp` and
  `src/backend/mir/aarch64/module/module.cpp` reports shared prepared object
  consumer violations but is not a direct freshness consumer.

x86 and shared-prealloc consumers reviewed but not wired:

- x86 scalar i32 call-argument handoff in `src/backend/mir/x86/x86.hpp` and
  `src/backend/mir/x86/module/module.cpp` remains on Route6 consumed source
  records through `find_consumed_scalar_i32_call_argument_source_authority`.
- x86 prepared move-bundle and decoded-home handoff paths remain protected by
  existing handoff short-circuit and `MissingValueAuthority` diagnostics.
- Shared-prealloc dependency operand authorities, edge-publication move
  consumers, select-carrier alias authorities, branch stack-load authorities,
  typed stack-source publications, recovered narrow store-source publication
  helpers, and pending store-global publication runs remain intentionally
  unwired. Step 5 selected store-source publication as the only
  producer/publication operand route for this MVP.

Existing fail-closed diagnostics protecting unwired consumers:

- AArch64 `MissingValueAuthority` diagnostics, including
  `AArch64 prior-preserved call argument requires prepared PriorPreservation source selection`
  and
  `AArch64 indirect byval call-argument publication requires complete prepared selected source bytes`,
  covered by `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`.
- Shared object-consumer violation categories reported through AArch64
  traversal, covered by
  `tests/backend/mir/backend_aarch64_function_traversal_test.cpp`.
- x86 consumed scalar source authority checks, covered by
  `tests/backend/bir/backend_x86_handoff_boundary_direct_extern_call_test.cpp`.
- x86 handoff short-circuit and decoded-home diagnostics, covered by
  `tests/backend/bir/backend_x86_handoff_boundary_scalar_smoke_test.cpp`,
  `tests/backend/bir/backend_x86_handoff_boundary_short_circuit_test.cpp`, and
  `tests/backend/bir/backend_x86_prepared_decoded_home_storage_test.cpp`.
- Shared-prealloc dependency operand stack-load routes remain fail-closed as
  `status=missing_stack_freshness`, covered in
  `tests/backend/bir/backend_prepared_printer_test.cpp`.

Focused stale-home, missing-producer, and move-source coverage:

- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp` covers selected,
  no-candidate, invalid, ambiguous, and unknown-use query statuses.
- `tests/backend/bir/backend_prepared_printer_test.cpp` proves producer
  freshness outranks prior preservation, prior preservation remains selected
  when it is the only valid source, and store-source publication rows expose
  selected `producer_rematerialization` authority for binary and
  select-materialization routes.
- `tests/backend/bir/backend_prepared_object_consumer_contract_test.cpp`
  proves the production move-bundle classifier does not manufacture source
  freshness and covers missing, invalid, ambiguous, and unsupported freshness
  diagnostics.
- `tests/backend/mir/backend_riscv_object_emission_test.cpp` covers selected
  prior-preservation freshness, missing and ambiguous freshness, producer
  outranking prior preservation, and `MissingMoveBundleSourceFreshness` with
  `prepared move-bundle source is missing freshness authority`.

Recommended follow-up ideas:

- AArch64 call-argument freshness migration for prior preservation,
  publication routing, byval/register-lane completeness, and target diagnostics.
- RV64 stack-source, frame-slot, and byval aggregate call-argument freshness
  policy.
- x86 Route6 consumed-source integration with the prepared freshness authority
  model.
- Broader shared-prealloc operand authority for dependency operands,
  edge-publication moves, select-carrier aliases, branch stack loads, typed
  stack-source publications, recovered narrow store-source publications, and
  pending store-global publication routes.

Closure proof:

```sh
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed
```

Result: passed with `passed=346 failed=0 total=346` before and after.
