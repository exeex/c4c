# Transitive Current-Block Incoming-Expression Authority Decomposition Runbook

Status: Active
Source Idea: ideas/open/718_transitive_current_block_incoming_expression_authority_decomposition.md
Supersedes active execution of: ideas/open/717_current_block_routed_value_authority_decomposition.md

## Purpose

Replace the blocked integration-driven route with focused probes for direct
source identity, transitive producer closure, zero-fact semantics, and upstream
stable-key owner-fact composition.

## Goal

Prove each authority seam independently before implementation or AArch64
integration resumes.

## Core Rule

Do not define authority in the AArch64 integration test. Preserve publication
source identity, and authorize dependencies only through a focused, general
semantic contract composed upstream into owner facts.

## Read First

- `ideas/open/718_transitive_current_block_incoming_expression_authority_decomposition.md`
- `ideas/open/717_current_block_routed_value_authority_decomposition.md`
- `todo.md`
- `src/backend/prealloc/publication_plans.cpp`
- `src/backend/prealloc/prepared_lookups.hpp`

## Current Targets

- focused registered probes under `tests/backend/case/`
- upstream prepared publication and stable-key owner-fact boundaries
- `backend_aarch64_current_block_join_routing` as integration-only proof

## Non-Goals

- Do not use Route 5, target-local reconstruction, or source-identity rewriting.
- Do not change supported integration vectors.
- Do not reopen accepted owner storage or lookup lifetime work.
- Do not close ideas 717, 716, 713, or 705 in this runbook.

## Execution Rules

- Keep exactly one primary semantic seam per focused probe.
- Extract and register probes before semantic implementation edits.
- Fail closed on missing, ambiguous, conflicting, cyclic, or incomplete facts.
- Treat probe registration and decomposition as route correction, not backend
  capability progress.
- For code-changing steps, run build, the named focused proof, and the exact
  supervisor-selected matching proof command.
- Require fresh broader backend proof before integration adoption and handback.

## Ordered Steps

### Step 1: Establish the blocked-family baseline and seam inventory

Goal: freeze the unchanged supported contract and map every collision to one
independently owned semantic seam.

Actions:

- Record the 317/320 consumer-only result and preparation trace showing the
  same `%source` plus `%operand` collision.
- Inventory direct publication identity, producer dependency edges, policy
  state, owner state, zero-fact state, stable keys, and composition boundaries.
- Select the supervisor-owned matching baseline/proof command.
- Confirm the integration test remains unchanged and integration-only.

Completion check:

- Every blocked fact maps to exactly one of the four source-idea seams, the
  baseline command is recorded, and no implementation or expectation changed.

### Step 2: Extract and register one focused probe per seam

Goal: create minimal backend-owned contracts before backend surgery.

Actions:

- Choose repo-conforming files under `tests/backend/case/` when supported.
- Extract separate probes for direct source identity, transitive dependency
  closure, no-policy/zero-fact semantics, and stable-key composition.
- Give each probe positive and fail-closed negative cases without copying the
  integration fixture or naming its vectors.
- Keep `backend_aarch64_current_block_join_routing` integration-only.

Completion check:

- Four focused probes are registered, each has one primary contract, and their
  initial results identify capability gaps without expectation weakening.

### Step 3: Bind each probe to one owned backend capability

Goal: locate the narrowest upstream ownership boundary for each contract.

Actions:

- Bind direct identity to preserved publication-source facts.
- Bind transitive closure to a deterministic producer-dependency rule with
  explicit termination and conflict handling.
- Bind no-policy/zero-fact behavior to an explicit semantic policy contract.
- Bind composition to stable-key owner-fact production upstream of AArch64.
- Reject any seam that can only be implemented through Route 5, target scans,
  source rewriting, or integration-vector matching.

Completion check:

- Each probe names one upstream capability owner and a semantic rule precise
  enough for an implementation packet; no AArch64 authority is required.

### Step 4: Implement direct publication-source identity authority

Goal: prove preserved publication identity without dependency conflation.

Actions:

- Implement the narrowest general upstream rule owned by the direct-source
  probe.
- Preserve source IDs and names; do not rewrite them to queried dependencies.
- Run build, focused proof, and the delegated matching subset.

Completion check:

- Direct source positives and negative identity conflicts are green with
  preserved source identity.

### Step 5: Implement transitive producer-dependency closure

Goal: authorize producer dependencies through a general, deterministic closure.

Actions:

- Implement closure at the Step 3 owner boundary.
- Prove multiple depths, termination, cycles, missing edges, ambiguity, and
  conflicting producer paths.
- Run build, focused proof, and the delegated matching subset.

Completion check:

- The dependency probe is green without fixed-depth, named-operand, or
  target-local shortcuts.

### Step 6: Implement no-policy and zero-owner-fact semantics

Goal: distinguish supported policy absence from missing authority.

Actions:

- Implement the explicit semantic contract identified in Step 3.
- Prove absent owner, absent policy, zero facts, and authoritative facts as
  distinct states.
- Do not make owner presence alone authoritative.
- Run build, focused proof, and the delegated matching subset.

Completion check:

- The focused state matrix is green and the supported zero-fact behavior has a
  semantic explanation independent of integration expectations.

### Step 7: Compose stable-key owner facts upstream

Goal: combine the three proven authorities into target-independent owner facts.

Actions:

- Implement composition only at the upstream owner-fact boundary from Step 3.
- Run all four focused probes together and inspect for hidden Route 5,
  source-rewrite, or target reconstruction paths.
- Run fresh broader backend proof before integration adoption.

Completion check:

- Focused and broader proof are green and stable-key owner facts represent each
  proven category without AArch64 authority construction.

### Step 8: Validate unchanged integration and hand back to idea 717

Goal: prove composition on the existing integration contract and resume the
blocked parent initiative.

Actions:

- Consume only owner-attached stable-key facts in AArch64.
- Run unchanged integration vectors plus fresh broader backend proof.
- Record the durable completed contract in idea 718 and switch execution back
  to idea 717 only when its handback criteria are satisfied.

Completion check:

- Unchanged integration and broader proof are green, idea 718 is ready to
  close, and idea 717 can resume without the transitive or zero-fact collision.
