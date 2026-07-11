# Current-Block Edge-Bound Routing Consumption Decomposition Runbook

Status: Active
Source Idea: ideas/open/713_current_block_edge_bound_routing_consumption_decomposition.md
Resumed after completion of: ideas/closed/716_prealloc_current_block_routing_authority_closure.md

## Purpose

Complete bounded result-level consumption using the proven prealloc-owned,
edge-bound authority contract before resuming Route 5 retirement in idea 705.

## Goal

Make AArch64 consume only the owner-attached stable-key query while prealloc
retains all authority, semantic identity, and ambiguity decisions.

## Core Rule

Do not reconstruct or choose routing authority in AArch64. A positive answer is
available only when the owner-prepared query proves complete semantic identity
and invariance across every applicable fact.

## Read First

- `ideas/open/713_current_block_edge_bound_routing_consumption_decomposition.md`
- `ideas/closed/716_prealloc_current_block_routing_authority_closure.md`
- `ideas/closed/718_prepared_routing_root_dependency_classification_decomposition.md`
- `review/step8_prepared_routing_handback_final_review.md`
- `src/backend/prealloc/prepared_lookups.hpp`
- `src/backend/mir/aarch64/codegen/dispatch_producers.cpp`
- `tests/backend/mir/backend_aarch64_current_block_join_routing_test.cpp`

## Proven Input Contract

- Owner attachment and copied-context lifetime do not grant routing authority.
- Route 5 is diagnostic-only and cannot seed, replace, erase, or authorize a
  routing fact.
- Prepared facts require complete edge identity, destination, routed stable
  key, available status, known semantic origin, and all-applicable agreement.
- Exact negative prepared roots propagate fail-closed state; legitimate
  uncovered BIR-PHI dependencies remain available.
- Duplicate and parallel disagreement remain ambiguous.
- AArch64 uses only the owner-attached stable-key query; unchanged integration
  contracts and the broader 329/329 backend proof are accepted.

## Non-Goals

- Do not resume idea 705 Route 5 payload removal before this handback closes.
- Do not migrate common MIR queries or unrelated targets.
- Do not add function-wide scans, pointer identity, first-match selection,
  result-name matching, target reconstruction, or expectation downgrades.
- Do not reopen idea 716's completed authority classification family unless a
  concrete contract regression is proven.

## Execution Rules

- Use registered focused tests as acceptance proof; keep the AArch64 test as
  integration proof.
- Treat commit `1394423de`, the final accepted Step 8 review, and 329/329
  backend proof as the authority baseline.
- Keep AArch64 limited to stable consumption identity and the attached lookup.
- Run build, focused/integration proof, and the supervisor-selected matching
  backend subset for any code-changing correction.

## Ordered Steps

### Steps 1-3: Define the edge-bound result-consumption query

Status: Complete through the accepted idea 716 handback.

Completion evidence:

- Registered contracts cover result-level identity, parallel predecessors,
  parallel destinations, wrong-successor isolation, duplicates, missing,
  incomplete, mismatched, and ambiguous facts.
- The query is prealloc-owned, stable-keyed, pointer-free, and requires
  semantic invariance across every applicable fact.

### Step 4: Correct and prove bounded AArch64 consumption

Goal: adopt and verify the accepted owner-attached query without reconstruction.

Actions:

- Verify `PreparedFunctionLookups::current_block_join_routing_facts` is built
  and attached at the prealloc/function-context owner boundary.
- Verify AArch64 transports only the stable consumption key and fails closed
  when the owner-attached lookup is absent.
- Verify no AArch64-triggered fallback calls a function-wide prepared lookup
  builder or reconstructs routing evidence.
- Preserve unchanged supported integration expectations and original producer
  semantics.
- Run the focused probes, AArch64 integration contract, and fresh matching
  broader backend proof if any correction is required.

Completion check:

- Owner-precomputed routing facts are the sole bounded AArch64 authority; no
  fallback or reconstruction path exists; supported integration remains
  unchanged; focused, integration, and broader backend proof are green.

### Step 5: Hand back to idea 705

Goal: preserve the bounded-consumption contract and resume the prepared fact
boundary initiative.

Actions:

- Record the completed query/consumer contract and proof in idea 713.
- Close idea 713 once its acceptance criteria and regression gate pass.
- Reactivate idea 705 at its repaired Step 2.3b.3 boundary.

Completion check:

- Idea 713 is closed and idea 705 resumes with no unresolved identity,
  authority, attachment, or AArch64-consumption ambiguity.
