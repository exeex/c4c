# Stack-Destination Fan-In Authority Decomposition

Status: Open
Type: Decomposition
Parent: `ideas/open/647_ordered_or_exclusive_stack_destination_fan_in_authority.md`
Related:
- `ideas/open/647_ordered_or_exclusive_stack_destination_fan_in_authority.md`
- `ideas/closed/607_destination_fan_in_authority_research.md`
- `ideas/closed/637_prepared_stack_destination_fan_in_authority_producer.md`
- `docs/destination_fan_in_authority/03_implementation_split.md`
Owning Layer: prepared/prealloc move-bundle destination authority production
Queue Order: 55
Proof Surface: focused probes for non-637 register-source fan-in to one stack
destination

## Goal

Split the blocked idea 647 Step 2 route into focused authority probes before
selecting another implementation family for stack-destination register fan-in.

## Why This Exists

Idea 647 is still valid, but its current execution route is too coarse. Step 2
has now rejected two first-family selections without producing a legal non-637
implementation packet:

- `src/20021204-1.c` mutual-exclusion authority is rejected because the failing
  `%t20/%t21 -> %t22` bundle has no predicate, edge,
  selected-active-candidate, guarded-copy, or consumer-point carrier fact.
- `src/20011109-2.c` has real select-materialized preserved-stack-fallback
  evidence, but that reopens the closed idea 637 contract rather than proving
  idea 647 progress.

The remaining residual rows need decomposition into owned seams before another
producer family is selected.

## In Scope

- Establish the blocked residual baseline from idea 647's refreshed Step 2
  evidence.
- Split non-637 stack-destination register fan-in residuals into focused seams:
  ordered final-state authority, mutual-exclusion authority, explicit merge
  authority, and unsupported or mismatched authority rejection.
- Define focused backend or prepared/prealloc probe files under
  `tests/backend/case/` for each seam before implementation.
- Bind each probe to a producer fact shape, owner label, negative statuses, and
  consumer program point.
- Select the first implementation follow-up only after one seam has positive
  producer evidence and at least one precise negative probe.

## Out Of Scope

- Reopening idea 637's select-materialized semantic-merge preserved-stack
  fallback contract.
- Implementing producer authority before focused seams and probes are named.
- RV64 target materialization for a new family before prepared/prealloc
  publishes an explicit destination authority fact.
- Rewriting expectations, unsupported markers, allowlists, timeouts, runtime
  behavior, or pass/fail accounting.
- Naming probes or fixes around GCC torture testcase identity instead of the
  underlying authority contract.

## Decomposition Seams

- Ordered final-state authority: prove that the producer designates one final
  authoritative stack-slot state at the consumer point.
- Mutual-exclusion authority: prove exactly one candidate write is active
  through predicate, edge, selected-active-candidate, guarded-copy, or carrier
  metadata at the consumer point.
- Explicit merge authority: prove semantic equivalence or an explicit merge
  operation for all candidate sources targeting the same stack destination.
- Rejection authority: prove missing, unsupported, stale, ambiguous, and
  bundle-versus-move mismatched authority shapes fail closed before target
  materialization.

## Expected Focused Probes

Use descriptive case names under `tests/backend/case/`; final filenames may
change during execution, but each file must own one seam:

- `riscv64_stack_destination_ordered_final_state_authority.c`
- `riscv64_stack_destination_mutual_exclusion_authority.c`
- `riscv64_stack_destination_explicit_merge_authority.c`
- `riscv64_stack_destination_authority_rejection.c`

The original GCC torture rows remain integration evidence only. They must not
be the first proof surface for a new authority family.

## Acceptance Criteria

- The blocked family baseline is recorded from the refreshed idea 647 evidence.
- Each seam has a focused probe design naming the expected positive or negative
  producer facts.
- At least one seam is selected as a legal follow-up implementation idea or
  packet with positive producer evidence and fail-closed negative evidence.
- Rows that still lack producer facts remain rejected with precise diagnostics.
- Idea 647 can later resume with a smaller, evidence-backed first family rather
  than another broad Step 2 search.

## Reviewer Reject Signals

- Reject treating this decomposition as implementation progress by itself.
- Reject selecting `SelectMaterializationPreservedStackFallback` or any other
  idea 637 contract as a non-637 idea 647 family.
- Reject reusing unrelated select, join, edge, or control-flow facts from one
  destination as authority for another destination.
- Reject testcase-shaped probes, named-case shortcuts, expectation rewrites, or
  unsupported-marker downgrades as progress.
- Reject RV64 materialization work before a prepared/prealloc producer fact is
  visible on the bundle and participating moves.
- Reject helper renames, classification-only edits, or diagnostic wording
  changes claimed as capability progress.
