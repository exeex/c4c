# Prealloc Current-Block Routing Authority Closure

Status: Closed
Type: backend authority decomposition
Blocks:
- `ideas/open/713_current_block_edge_bound_routing_consumption_decomposition.md`
- `ideas/open/705_prepared_fact_boundary_from_bir_views.md`

## Goal

Close the prealloc current-block routing authority boundary with focused,
registered contracts before bounded AArch64 consumption resumes.

## Why This Idea Exists

Idea 713 Step 4 collided twice with the same authority problem. The first route
moved lookup construction to the function-context owner but retained a target
fallback and weakened integration expectations. The correction removed the
fallback and established owner attachment, but then derived `Available`
incoming-expression facts from a unique Route 5 index and changed the
fixture's policy-versus-attachment meaning.

The owner lifetime correction is useful and separable. It must not make Route
5 diagnostic identity authoritative, replace complete edge-derived facts, or
allow unchanged integration vectors to conceal changed fixture semantics.
Ideas 713 and 705 remain open but blocked pending handback from this initiative.

## In Scope

- Prove the owner attachment and lookup lifetime contract independently of
  routing semantics.
- Prove Route 5 remains diagnostic-only and cannot seed, replace, erase, or
  authorize incoming-expression routing facts.
- Define complete edge-derived incoming-expression authority, including
  predecessor, destination, semantic origin, and invariance across every
  applicable fact.
- Separate integration fixture policy construction from lookup attachment so
  positives and fail-closed negatives state distinct contracts.
- Resume bounded AArch64 consumption only after all authority probes are green.

## Primary Registered Contracts

Prefer focused C++ contracts because these are internal ownership seams:

1. `tests/backend/mir/backend_prealloc_current_block_lookup_attachment_lifetime_test.cpp`
   proves owner attachment and copied-context lifetime without asserting a
   routing answer.
2. `tests/backend/bir/backend_prealloc_route5_diagnostic_non_authority_test.cpp`
   proves Route 5 agreement or unique identity cannot create or replace an
   incoming-expression fact.
3. `tests/backend/bir/backend_prealloc_current_block_incoming_expression_authority_test.cpp`
   proves predecessor, destination, semantic origin, and all-applicable-edge
   invariance are required for `Available`.
4. `tests/backend/mir/backend_aarch64_current_block_fixture_policy_attachment_test.cpp`
   proves policy presence and lookup attachment as independent fixture axes.
5. `tests/backend/mir/backend_aarch64_current_block_join_routing_test.cpp`
   remains the bounded AArch64 integration contract and is not a discovery
   surface.

Equivalent existing registered contract files may be extended when they
already own exactly one named seam; do not combine seams to reduce file count.

## Out Of Scope

- Route 5 payload retirement or broader idea 705 migration.
- Common MIR query migration or unrelated target materializers.
- AArch64 reconstruction, function-wide scans, pointer identity, unique
  Route 5 selection, or result-name matching as routing authority.
- Rewriting supported integration vectors, changing policy meaning to retain
  those vectors, or treating attachment as evidence that policy exists.
- Broad publication-plan or BIR schema redesign beyond the five contracts.

## Acceptance Criteria

- The attachment/lifetime contract is green without depending on Route 5 or a
  positive routing result.
- Route 5 can vary, agree, or uniquely identify a diagnostic source without
  changing the authoritative incoming-expression fact set.
- `Available` is derived only from complete prepared edge facts and requires
  predecessor, destination, semantic origin, and invariance across all
  applicable facts; missing, ambiguous, and mismatched states remain explicit.
- Fixture policy and attachment are independently controllable and registered
  positives contain real edge-derived authority.
- The bounded AArch64 consumer reads only the owner-attached stable-key query,
  has no reconstruction fallback, and passes unchanged supported integration
  expectations after the four focused contracts are green.
- Fresh focused and broader backend proof supports handback to idea 713 Step 4;
  idea 705 remains blocked until idea 713 completes its own handback.

## Reviewer Reject Signals

- A unique Route 5 index, route agreement, result name, or successor-only key
  seeds, replaces, filters, or authorizes an incoming-expression fact.
- An `Available` fact omits predecessor, destination, semantic origin, or the
  proof that all applicable edge facts agree.
- Owner attachment is claimed as routing correctness without an independent
  complete-edge authority probe.
- The integration fixture changes policy construction, attachment behavior, or
  supported vectors to preserve a named case instead of proving the contract.
- A detached negative is used to excuse a positive whose policy authority is
  absent or Route 5-derived.
- Helper renames, expectation rewrites, or classification-only changes are
  claimed as capability progress.
- The exact Route 5-to-`Available` promotion survives behind a new abstraction.
- The route broadens into unrelated publication, BIR, MIR, or target work.

## Completion Note

Closed after the accepted authority handback in commit `1394423de` and the
final Step 8 review. Owner attachment and copied-context lifetime are proven
independently; Route 5 remains diagnostic-only; complete prepared authority
requires predecessor/successor, destination, routed stable key, available
status, known semantic origin, and agreement across every applicable fact.
Exact negative prepared roots remain fail closed, legitimate uncovered
BIR-PHI dependencies remain available, and duplicate or parallel disagreement
remains ambiguous. Fixture policy and attachment stay independent, AArch64
consumes only the owner-attached stable-key query with no reconstruction
fallback, unchanged supported vectors and original short-circuit producers
pass, and the accepted broader backend proof is 329/329. The authority contract
is handed back to idea 713 Step 4; idea 705 remains blocked on idea 713.
