# Ordered Or Exclusive Stack-Destination Fan-In Authority

Status: Open
Type: Implementation
Parent: `ideas/closed/637_prepared_stack_destination_fan_in_authority_producer.md`
Related:
- `ideas/closed/607_destination_fan_in_authority_research.md`
- `ideas/closed/637_prepared_stack_destination_fan_in_authority_producer.md`
- `docs/destination_fan_in_authority/03_implementation_split.md`
Owning Layer: prepared/prealloc move-bundle destination authority production
Queue Order: 47
Proof Surface: residual non-parallel register-source fan-in to one stack
destination that is not covered by the select-materialized semantic-merge
contract

## Goal

Define the next explicit prepared/prealloc destination authority family for
stack-destination register fan-in residuals that are outside idea 637's
selected semantic-merge contract.

## Why This Exists

Idea 637 closed after proving exactly one producer authority family:
select-materialized semantic merge with preserved stack fallback. The original
six spillover rows remain fail-closed because they require a different
authority family rather than another packet under the selected contract.

Residual rows:

- `src/20011109-2.c`: `authority=none`, `move_count=3`, and
  `fragment_status=missing_stack_destination_fan_in_authority_fact`.
- `src/20021204-1.c`, `src/920429-1.c`, `src/930429-1.c`,
  `src/pr34415.c`, and `src/ptr-arith-1.c`: non-parallel
  two-register-source stack-destination fan-ins with `authority=none` and
  `fragment_status=producer_authority_missing_for_register_fan_in_stack_destination`.
- `src/pr70005.c`: routed from idea 640 after scalar frame-slot local-memory
  publication advanced; current first owner is
  `unsupported_prepared_move_bundle_classification` for an ambiguous
  non-parallel multi-source stack destination in `fn1` at `logic.end.73`.

These rows should be investigated as ordered final-state or mutual-exclusion
producer authority, not as select-materialized semantic merge.

## In Scope

- Refresh diagnostics for the six residual rows and classify which require
  ordered final-state authority, mutual-exclusion authority, or another
  explicitly named destination authority family.
- Choose exactly one first producer family for this idea before implementation.
- Publish prepared/prealloc facts only when the producer can prove that family
  at the consumer program point.
- Preserve fail-closed diagnostics for missing, unsupported, ambiguous, stale,
  and bundle-versus-move mismatched destination authority.
- Add focused prepared/prealloc or backend tests that prove one legal shape for
  the selected family and one missing-authority rejection.

## Out Of Scope

- Reopening idea 637's select-materialized semantic-merge contract.
- RV64 target materialization for a newly authorized residual family unless a
  downstream consumer idea is separately active.
- Inferring destination authority from filenames, source order, value ids,
  block labels, diagnostics, move-vector order, source freshness, final
  assembly, ABI, runtime behavior, expectations, allowlists, timeouts, or
  unsupported markers.
- String-constant local-memory policy.

## Acceptance Criteria

- The residual rows are refreshed and classified under a concrete
  destination-authority family.
- One selected family is represented in prepared/prealloc facts with an
  explicit owner and negative states.
- At least one focused legal fan-in shape publishes the selected authority, or
  the route records precise producer evidence proving no legal first packet is
  available.
- Missing or mismatched authority remains fail-closed with precise diagnostics.
- Residuals outside the selected family are either left fail-closed with
  durable notes or moved to a separate open idea.

## Reviewer Reject Signals

- Reject treating this idea as another packet for idea 637's
  select-materialized semantic-merge contract.
- Reject named-case-only fixes for the six residual rows.
- Reject RV64-side source selection before prepared/prealloc publishes explicit
  destination authority for the selected family.
- Reject using source freshness, string-label pointer authority, frame slot
  existence, move order, final assembly, diagnostics, or testcase identity as
  destination fan-in authority.
- Reject expectation rewrites, unsupported-marker changes, allowlist edits,
  timeout/accounting changes, runtime changes, helper renames, or
  classification-only edits claimed as capability progress.

## Parked Outcome

The active runbook was parked after Step 2 in commit `ea0ab226f` because the
refreshed residual baseline did not expose a legal positive non-637 producer
authority family.

Durable evidence:

- Step 1 refreshed the residual set in
  `build/agent_state/647_step1_20260710_residual_baseline/summary.md`.
- Step 2 selected no implementation family: ordered final-state and explicit
  merge have no producer metadata at the consumer program point; mutual
  exclusion remains negative for `src/20021204-1.c`; `src/20011109-2.c`
  re-enters closed idea 637 evidence; `src/920429-1.c`,
  `src/ptr-arith-1.c`, and `src/pr70005.c` are rejection-only; and
  `src/930429-1.c` plus `src/pr34415.c` are stale for this route.
- The latest proof command was
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`,
  recorded as passing in `test_after.log`.

Leave this idea parked until new prepared/prealloc evidence exposes a positive
non-637 ordered final-state, mutual-exclusion, or explicit-merge producer fact
at the failing consumer program point. Do not route to implementation from the
parked runbook alone.
