# Prepared Stack-Destination Fan-In Authority Producer

Status: Closed
Type: Implementation
Parent: `ideas/closed/607_destination_fan_in_authority_research.md`
Related:
- `ideas/closed/607_destination_fan_in_authority_research.md`
- `ideas/closed/584_rv64_stack_destination_move_bundle_authority_contract.md`
- `ideas/closed/630_string_constant_local_memory_policy.md`
- `docs/destination_fan_in_authority/03_implementation_split.md`
Owning Layer: prepared/prealloc move-bundle destination authority production
Queue Order: 37
Prerequisites: choose exactly one producer authority contract from the
destination fan-in research before RV64 consumption changes
Proof Surface: non-parallel register-source fan-in to one stack destination
after rows already carry any required source freshness and unrelated local
memory authority

## Goal

Define and publish one explicit prepared/prealloc destination authority
contract for non-parallel register-source fan-in to a single stack destination.

## Why This Exists

Idea 630 closed after all ten string-constant representative rows published
`StringConstantLabelPointer` authority. Six rows then moved to the separate
prepared move-bundle fan-in owner:
`src/20011109-2.c`, `src/20021204-1.c`, `src/920429-1.c`,
`src/930429-1.c`, `src/pr34415.c`, and `src/ptr-arith-1.c`.

The destination fan-in research in idea 607 already selected explicit
rejection as the current rule: RV64 must not choose among multiple register
sources targeting one stack destination unless prepared/prealloc producer
metadata proves ordering, mutual exclusion, or merge authority at the consumer
program point.

## In Scope

- Refresh current diagnostics for the six idea-630 spillover rows and one or
  more wider destination fan-in representatives from the 607 evidence.
- Choose exactly one first producer contract: ordered final-state authority,
  mutual-exclusion authority, or semantic merge authority.
- Publish destination authority facts on prepared move bundles and participating
  moves, while keeping source freshness checked separately.
- Preserve fail-closed diagnostics for `authority=none`, stale source
  freshness, missing final-state or predicate evidence, unsupported authority
  kinds, and bundle-versus-move mismatches.
- Add focused prepared/prealloc or backend tests that prove one supported legal
  fan-in shape and one missing-authority rejection.

## Out Of Scope

- RV64 target materialization for newly authorized fan-in; that is a downstream
  consumer idea after producer authority exists.
- Inferring destination legality from testcase names, source order, value ids,
  block labels, final assembly, move-vector order, or diagnostic strings.
- Reusing move-bundle source freshness as destination authority.
- String-constant local-memory policy; idea 630 is closed.
- ABI, runtime, expectation, unsupported-marker, allowlist, timeout, or
  accounting changes.

## Acceptance Criteria

- A refreshed probe identifies the current fan-in rows and proves they are
  blocked by missing producer destination authority rather than by string
  constant local-memory admission.
- One concrete destination authority family is represented in prepared/prealloc
  facts with an explicit owner and negative states.
- At least one focused legal fan-in shape publishes the new authority and moves
  past the producer-authority-missing diagnostic, or the route records why no
  legal first packet exists under the selected contract.
- Missing, unsupported, ambiguous, stale, or mismatched destination authority
  remains fail-closed with precise diagnostics.

## Closure Notes

Closed after selecting exactly one producer authority family:
semantic merge / select-materialized destination fan-in, represented by
`PreparedMoveAuthorityKind::StackDestinationRegisterFanIn` plus
`PreparedStackDestinationFanInSemantics::SelectMaterializationPreservedStackFallback`.

Current code already publishes and consumes that producer-side fact for the
selected shape. Focused legal coverage verifies the prepared authority owner
`prepared_stack_destination_register_fan_in` and RV64 object emission for the
authorized fixture. Focused negative coverage verifies missing, malformed,
bundle-only, unsupported, unknown, ambiguous, and non-select fan-in authority
states remain fail-closed rather than inferred from source freshness,
diagnostics, ordering, or final assembly.

The six original spillover rows are not part of the selected semantic-merge
contract and should not reopen this idea:

- `src/20011109-2.c`: `authority=none`, `move_count=3`, and
  `fragment_status=missing_stack_destination_fan_in_authority_fact`.
- `src/20021204-1.c`, `src/920429-1.c`, `src/930429-1.c`,
  `src/pr34415.c`, and `src/ptr-arith-1.c`: non-parallel
  two-register-source stack-destination fan-ins with `authority=none` and
  `fragment_status=producer_authority_missing_for_register_fan_in_stack_destination`.

Those residuals moved to
`ideas/open/647_ordered_or_exclusive_stack_destination_fan_in_authority.md`,
which owns the different destination-authority family question: ordered
final-state or mutual-exclusion producer authority.

Close validation used the existing matching canonical focused logs:
`test_before.log` and `test_after.log` for `backend_prepare_stack_layout`.
Both logs pass the same one-test scope. Because this closure is lifecycle-only
and introduced no implementation diff, the regression comparison was accepted
with non-decreasing pass count.

## Reviewer Reject Signals

- Reject RV64-side selection of a stack-destination source without explicit
  prepared/prealloc destination authority.
- Reject named-case-only fixes for any row listed in this idea.
- Reject treating source freshness, string-label pointer authority, or frame
  slot existence as proof of destination fan-in legality.
- Reject expectation rewrites, unsupported-marker changes, allowlist edits,
  timeout/accounting changes, or runtime behavior changes as progress.
- Reject helper renames or diagnostic-only edits that leave
  `authority=none` as the effective first owner for legal fan-in shapes.
