# Current-Block Edge-Bound Routing Consumption Decomposition Runbook

Status: Active
Source Idea: ideas/open/713_current_block_edge_bound_routing_consumption_decomposition.md
Supersedes active execution of: ideas/open/705_prepared_fact_boundary_from_bir_views.md

## Purpose

Decompose the blocked result-level versus edge-bound routing interface before
resuming Route 5 retirement in idea 705.

## Core Rule

Improve the proof and ownership boundary first. Do not patch AArch64 routing or
collapse edge facts until registered probes define when a result-level answer
is semantically unique.

## Read First

- `ideas/open/713_current_block_edge_bound_routing_consumption_decomposition.md`
- `ideas/open/705_prepared_fact_boundary_from_bir_views.md`
- `review/reviewA.md` if present in history
- `src/backend/prealloc/publication_plans.hpp`
- `src/backend/mir/aarch64/codegen/dispatch_producers.cpp`
- `tests/backend/mir/backend_aarch64_current_block_join_routing_test.cpp`

## Non-Goals

- Do not resume idea 705 Route 5 public payload removal during baseline/probe work.
- Do not migrate common MIR queries or unrelated targets.
- Do not use function-wide scans, pointer identity, first-match selection, or
  expectation downgrades.
- Do not treat lifecycle switching as backend capability progress.

## Execution Rules

- Use registered tests only as acceptance proof.
- Keep one primary identity/collision contract per probe.
- Preserve the existing AArch64 routing test as integration proof, not the
  discovery surface.
- Bind every proposed aggregate/query to explicit ownership and negative status.
- Run build plus focused proof for each step and the broader backend proof at
  integration.

## Ordered Steps

### Step 1: Establish the result-level versus edge-bound baseline

Goal: capture the blocked interface mismatch in one registered focused contract
without changing backend selection.

Actions:

- Record the stable identities available to the result-level consumer and the
  additional predecessor/destination identities owned by prealloc facts.
- Add a registered baseline proving that one BIR result can correspond to more
  than one valid edge-bound fact.
- Keep the current AArch64 integration test unchanged.

Completion check:

- A registered focused test deterministically reproduces the mismatch and the
  proof names the first identity lost between prealloc and the consumer.

### Step 2: Extract collision-family probes

Goal: split the blocked family into independent, registered contracts.

Actions:

- Add one-primary-contract probes for parallel predecessors, parallel
  destinations, wrong successor, and duplicate semantic edge.
- Assert explicit available, missing, ambiguous, or mismatched outcomes without
  target code changes.
- Reject probes that are smaller copies of the integration fixture but do not
  isolate one ownership seam.

Completion check:

- All four collision families have reachable registered proof and no probe
  depends on Route 5 agreement.

### Step 3: Define the prealloc-owned result-consumption query

Goal: map edge-bound facts to the result-level consumption point without moving
authority into the target.

Actions:

- Define the stable query key available at the consumer.
- Return a positive aggregate only when prealloc proves the requested routing
  answer is invariant across all applicable edge-bound facts.
- Preserve explicit missing, ambiguous, and mismatched results.
- Prove the query against every Step 2 collision probe before target adoption.

Completion check:

- The query is prealloc-owned, pointer-free, collision-complete, and green for
  all focused probes without function-wide target scans.

### Step 4: Prove bounded AArch64 consumption

Goal: make the bounded consumer read only the Step 3 query result.

Actions:

- Transport only the stable result-consumption key into prealloc.
- Remove target-side publication/`JoinTransfer` scans or evidence construction.
- Run focused probes, the existing AArch64 integration test, and the broader
  backend proof.

Completion check:

- AArch64 consumes prealloc authority without replanning; all focused and
  integration proof is green; and idea 705 can resume at Route 5 retirement.

### Step 5: Hand back to idea 705

Goal: preserve the proven query contract and switch lifecycle execution back to
the still-open prepared fact boundary idea.

Actions:

- Record the completed contract and proof in this idea.
- Switch the active plan back to idea 705 at its repaired Step 2.3b.3 boundary.

Completion check:

- Idea 713 is ready to close and idea 705 resumes with no unresolved identity
  ambiguity.
