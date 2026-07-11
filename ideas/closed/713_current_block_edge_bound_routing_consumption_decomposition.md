# Current-Block Edge-Bound Routing Consumption Decomposition

Status: Closed
Type: backend contract decomposition
Unblocks: `ideas/open/705_prepared_fact_boundary_from_bir_views.md`

## Goal

Define and prove the narrow contract that lets a result-level backend consumer
use prealloc-owned, edge-bound current-block routing facts without granting the
target publication authority or collapsing distinct edges ambiguously.

## Why This Idea Exists

Idea 705 Step 2.3b.3 reached the same interface mismatch twice. Prealloc owns
routing facts keyed by predecessor, successor, destination, source, move,
publication, freshness, and semantic origin. The bounded AArch64 API asks one
boolean question per BIR result and does not supply predecessor or destination.
Multiple valid edge facts can therefore share the same result identity.

The rejected route merged function-wide publications or `JoinTransfer` values
inside AArch64. `review/reviewA.md` correctly rejected that as target-side
authority and an overfit risk. More Route 5 retirement work must wait until the
identity mismatch has focused, registered proof and a prealloc-owned solution.

## In Scope

- Establish a registered baseline that distinguishes result-level queries from
  edge-bound prepared identity.
- Extract one-primary-contract probes for:
  - one result used by parallel predecessors
  - one result routed to parallel destinations
  - wrong-successor isolation
  - duplicate semantic edge rejection
- Define a narrow prealloc-owned aggregate/query that maps edge-bound facts to
  the target consumption point while retaining ambiguity and mismatch status.
- Prove the bounded AArch64 consumer can read that query without constructing
  evidence, scanning publications/transfers, or choosing publication semantics.
- Preserve the existing AArch64 current-block routing test as integration proof
  only after focused probes are green.

## Expected Probe Files

Prefer one registered backend case or contract per seam, for example:

- `tests/backend/case/current_block_result_edge_identity_baseline.c`
- `tests/backend/case/current_block_parallel_predecessor_source.c`
- `tests/backend/case/current_block_parallel_destination_source.c`
- `tests/backend/case/current_block_wrong_successor_isolation.c`
- `tests/backend/case/current_block_duplicate_semantic_edge.c`

If repository-native C++ contract tests are the smaller proof surface, use
separate registered tests under `tests/backend/bir/` or `tests/backend/mir/`
with the same one-primary-contract split. Do not rely on an unregistered source.

## Out Of Scope

- Removing Route 5 public payload from idea 705 before this contract is proven.
- Common MIR query migration.
- Unrelated AArch64, x86, or RV64 materializers.
- Function-wide `any_of`, first-match, pointer-identity, or value-name-only
  routing authority.
- Synthesizing a BIR CFG-edge publication relation for prepared-originated
  `JoinTransfer` behavior.
- Rewriting supported expectations to make ambiguous cases disappear.

## Acceptance Criteria

- Registered probes reproduce result-level versus edge-bound identity mismatch
  and distinguish all four collision families.
- The prealloc query consumes uniquely selected prepared facts and returns
  explicit available, missing, ambiguous, and mismatched status.
- Parallel predecessor/destination facts sharing a result cannot be silently
  merged, while a semantically safe result-level aggregate is available only
  when prealloc can prove the requested answer is invariant across applicable
  edges.
- AArch64 supplies only stable consumption-point identity and consumes the
  prealloc answer without scanning publications, `JoinTransfer`s, or BIR route
  indexes.
- Focused probes, the existing AArch64 integration test, and the broader backend
  proof are green before idea 705 resumes Step 2.3b.3.

## Reviewer Reject Signals

- A target change uses `any_of`, first match, value-name matching, or a
  function-wide publication/`JoinTransfer` scan to turn multiple edge facts
  into one boolean.
- A new query accepts only the named fixture shape and lacks parallel
  predecessor, parallel destination, wrong successor, and duplicate-edge
  negatives.
- Pointer equality is treated as semantic uniqueness.
- Tests downgrade a supported path, remove an ambiguity expectation, or merely
  rename Route 5 fields while retaining the same authority.
- An unregistered test source or the integration test alone is claimed as the
  focused proof.
- The implementation broadens into common MIR or unrelated target migration.
- The exact old failure mode survives behind a new aggregate/query name.

## Authority Handback

Idea 716 closed the prealloc authority dependency at commit `1394423de` with
final accepted review and 329/329 backend proof. The owner-attached stable-key
query is now the only AArch64 consumption boundary: Route 5 is diagnostic-only,
exact negative prepared roots fail closed, legitimate uncovered BIR-PHI
dependencies are preserved, and duplicate or parallel disagreement remains
ambiguous. Resume at Step 4 to adopt and verify this bounded contract; do not
reconstruct authority in the target or reopen the completed classification
family.

## Completion Note

Closed after the final Step 4 bounded-consumption review accepted commit
`1394423de` without another code packet. Registered contracts cover the
result-level/edge-bound identity mismatch, parallel predecessors and
destinations, wrong-successor isolation, duplicates, and explicit missing,
incomplete, mismatched, and ambiguous outcomes. The prealloc-owned stable-key
query proves semantic invariance across all applicable facts. AArch64 supplies
only stable consumption identity, reads the owner-attached query, fails closed
when the owner is absent, and does not scan publications, `JoinTransfer`
records, or BIR route indexes. Unchanged integration and the focused authority
probes pass in the accepted 329/329 backend proof. The bounded consumer
contract is handed back to idea 705 at Step 2.3b.3.
