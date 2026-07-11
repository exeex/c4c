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
- Extract and register each probe before implementing the behavior it judges.
  For the zero-policy seam only, first add the minimal upstream state boundary
  needed to make policy absence and attached-zero-fact state observable, then
  extract its probe before implementing routing semantics for either state.
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

### Step 2: Extract and register the three currently expressible focused probes

Goal: preserve the three valid backend-owned contracts already expressible by
production state while leaving the policy-axis probe pending its prerequisite.

Actions:

- Choose repo-conforming files under `tests/backend/case/` when supported.
- Extract separate probes for direct source identity, transitive dependency
  closure, and stable-key composition.
- Give each probe positive and fail-closed negative cases without copying the
  integration fixture or naming its vectors.
- Record explicitly that the no-policy/zero-fact probe cannot be truthful until
  upstream production state distinguishes policy absence from an attached
  owner with zero applicable facts; do not substitute owner attachment or a
  second composition collision for that axis.
- Keep `backend_aarch64_current_block_join_routing` integration-only.

Completion check:

- Three focused probes are registered with one primary contract each, their
  initial results identify capability gaps without expectation weakening, and
  the fourth probe's upstream state prerequisite is precise.

### Step 3: Define the upstream policy-state boundary

Goal: make the zero-policy seam representable without granting routing
authority or encoding it in an AArch64 fixture.

Actions:

- Add the narrowest upstream prepared-state representation and query boundary
  that distinguishes absent policy, absent owner, attached owner with zero
  applicable facts, and attached owner with authoritative facts.
- Keep the boundary target-independent and fail closed for missing, ambiguous,
  conflicting, or incomplete state.
- Do not make owner presence, empty vectors, or the new state representation
  itself authorize an incoming expression.
- Run build and the exact supervisor-selected matching proof command; existing
  supported expectations must remain unchanged.

Completion check:

- Production state can observe all four policy/owner states distinctly without
  AArch64 authority, and no routing behavior has been added or weakened.

### Step 4: Extract and register the no-policy/zero-fact focused probe

Goal: establish the fourth independent backend-owned contract before routing
semantics are implemented for the new state boundary.

Actions:

- Register a focused probe under `tests/backend/case/` against the Step 3
  production representation/query rather than fixture attachment.
- Encode positive and fail-closed negative cases for absent owner, absent
  policy, attached owner with zero facts, and authoritative facts.
- Keep one primary state-semantics seam; do not duplicate the stable-key
  composition collision or copy integration vectors.

Completion check:

- The fourth focused probe is registered, distinguishes the required state
  matrix, and exposes any semantic capability gap without expectation
  weakening.

### Step 5: Bind each probe to one owned backend capability

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

### Step 6: Implement direct publication-source identity authority

Goal: prove preserved publication identity without dependency conflation.

Actions:

- Implement the narrowest general upstream rule owned by the direct-source
  probe.
- Preserve source IDs and names; do not rewrite them to queried dependencies.
- Run build, focused proof, and the delegated matching subset.

Completion check:

- Direct source positives and negative identity conflicts are green with
  preserved source identity.

### Step 7: Implement transitive producer-dependency closure

Goal: authorize producer dependencies through a general, deterministic closure.

Actions:

- Implement closure at the Step 5 owner boundary.
- Prove multiple depths, termination, cycles, missing edges, ambiguity, and
  conflicting producer paths.
- Run build, focused proof, and the delegated matching subset.

Completion check:

- The dependency probe is green without fixed-depth, named-operand, or
  target-local shortcuts.

### Step 8: Implement no-policy and zero-owner-fact semantics

Goal: distinguish supported policy absence from missing authority.

Actions:

- Implement the explicit semantic contract identified in Step 5 through the
  state boundary established in Step 3.
- Prove absent owner, absent policy, zero facts, and authoritative facts as
  distinct states.
- Do not make owner presence alone authoritative.
- Run build, focused proof, and the delegated matching subset.

Completion check:

- The focused state matrix is green and the supported zero-fact behavior has a
  semantic explanation independent of integration expectations.

### Step 9: Compose stable-key owner facts upstream

Goal: combine the three proven authorities into target-independent owner facts.

Actions:

- Implement composition only at the upstream owner-fact boundary from Step 5.
- Run all four focused probes together and inspect for hidden Route 5,
  source-rewrite, or target reconstruction paths.
- Run fresh broader backend proof before integration adoption.

Completion check:

- Focused and broader proof are green and stable-key owner facts represent each
  proven category without AArch64 authority construction.

### Step 10: Validate unchanged integration and hand back to idea 717

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
