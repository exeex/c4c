# Prepared Routing Root/Dependency Classification Decomposition

Status: Open
Type: backend authority decomposition
Blocks:
- `ideas/open/716_prealloc_current_block_routing_authority_closure.md`

## Goal

Separate direct publication-root classification from composed dependency
authority so the owner query can reject an unrelated direct root without
discarding a legitimate dependency needed by a routed expression.

## Why This Idea Exists

Idea 716 Step 6.2 repeatedly reaches the same authority collision after its
unchanged integration contracts are restored. In the policy-present routing
fixture, direct `MismatchedSource` value 811 is incorrectly authorized as an
incoming expression, while composed dependency 810 must remain authorized.
Filtering the direct root at the current owner-query boundary also rejects the
composed dependency. The restored short-circuit fixture independently fails
with its original `%rhs.add` and `%short.selected` producers present.

The prior dirty candidate proved useful generic pieces, including owner-only
consumption, publication-origin preparation, a policy-absent negative, and
complete-transfer fail-closed checks. It is unaccepted evidence, not progress:
the final acceptance review rejected the accompanying supported-vector rewrites
and producer deletion. Continuing from the integration fixture would invite
another testcase-shaped classification rule, so the authority family must be
decomposed into focused registered contracts first.

## In Scope

- Classify direct publication roots independently from operands that are
  legitimate dependencies of a composed routed value.
- Prove the stable-key owner query preserves a legitimate composed dependency
  while rejecting a mismatched direct root for the same routing context.
- Prove memory-backed source/home routing under its own authority contract.
- Prove short-circuit select dependency composition with the original producer
  chain represented independently of the large integration fixture.
- Preserve complete destination, semantic-origin, and all-applicable-fact
  agreement requirements, including missing, mismatched, ambiguous, duplicate,
  multiple, and parallel fail-closed cases.
- Return to idea 716 only after all focused seams are green, then rerun the
  unchanged AArch64 integration contracts.

## Focused Registered Contracts

Prefer one backend-owned probe per seam under `tests/backend/case/`:

1. `tests/backend/case/current_block_direct_root_classification_probe.cpp`
   proves a mismatched direct publication root is not incoming-expression
   authority.
2. `tests/backend/case/current_block_composed_dependency_authority_probe.cpp`
   proves a legitimate operand dependency remains authoritative when its
   enclosing direct root is rejected.
3. `tests/backend/case/current_block_memory_source_authority_probe.cpp`
   proves memory-backed source/home routing independently.
4. `tests/backend/case/current_block_short_circuit_dependency_probe.cpp`
   proves composition through the original add/select dependency chain.

Equivalent focused registered paths are acceptable when repository test
registration requires them. Existing probes may be extended only when they
already own exactly the same single seam.

## Out of Scope

- Reclassifying idea 716's supported vectors or changing its fixture semantics.
- Deleting, bypassing, or replacing the original short-circuit producers.
- AArch64 reconstruction, target-local scans, Route 5 authority, result-name
  matching, successor-only identity, or named-value special cases.
- Treating owner attachment, memory homes, or dependency membership alone as
  sufficient incoming-expression authority.
- Broad publication-plan, BIR schema, MIR, or unrelated target redesign.
- Claiming the existing dirty implementation or test candidates as accepted
  progress before the focused contracts and unchanged integration proof pass.

## Acceptance Criteria

- Each of the four classification/dependency seams has one focused registered
  contract and one explicitly owned backend authority boundary.
- The owner query rejects mismatched direct root 811 while preserving composed
  dependency 810 through a generic classification rule with no numeric or
  fixture-shaped matching.
- Memory-backed source/home routing and short-circuit composition each pass
  independently, with missing or inconsistent authority failing closed.
- Complete transfer destination, semantic origin, and agreement across every
  applicable fact remain mandatory.
- The unchanged policy-present routing vectors and the instruction-dispatch
  fixture with `%rhs.add` and `%short.selected` present pass only after the
  focused probes are green.
- Fresh focused and broader backend proof supports handback to idea 716; idea
  716 remains open until its own acceptance criteria are satisfied.

## Reviewer Reject Signals

- A rule mentions values 810/811, fixture row names, `%rhs.add`,
  `%short.selected`, or equivalent named-case identity to decide authority.
- A direct root is made negative by filtering all operands or facts needed to
  preserve a legitimate composed dependency.
- Dependency membership, memory-home presence, attachment, Route 5 identity,
  result-name matching, or successor-only identity directly grants authority.
- A supported integration vector is rewritten, policy meaning changes, or an
  original producer is removed to obtain green proof.
- The focused probes are smaller copies of the integration fixture rather than
  independent single-seam contracts.
- Missing, incomplete, mismatched, ambiguous, duplicate, multiple, or parallel
  facts cease to exercise all-applicable-fact fail-closed behavior.
- Helper renames, expectation rewrites, fixture edits, or classification-only
  assertions are claimed as capability progress without a generic owner-bound
  rule and focused proof.
- The existing rejected dirty slice is committed or described as progress
  before its candidate pieces survive the decomposed contracts and unchanged
  integration proof.

## Handback Contract

When the four focused seams and broader proof are green, close this
decomposition initiative and reactivate idea 716 at its owner-publication and
bounded-consumption boundary. Hand back the generic classification rule,
registered proof names, and explicit remaining integration status; do not
silently broaden idea 716 while this initiative is active.
