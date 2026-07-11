# Current-Block Routed-Value Authority Decomposition Runbook

Status: Active
Source Idea: ideas/open/717_current_block_routed_value_authority_decomposition.md
Supersedes active execution of: ideas/open/716_prealloc_current_block_routing_authority_closure.md

## Purpose

Replace the blocked Step 6.2 route with focused prepared-fact authority probes
before returning to owner-only AArch64 consumption.

## Goal

Prove destination consistency, routed-operand identity, and all-edge invariance
as independent contracts, then compose them without changing supported vectors.

## Core Rule

Do not discover or define authority in the AArch64 integration test. A routed
value is available only under rules already proven by the focused BIR probes.

## Read First

- `ideas/open/717_current_block_routed_value_authority_decomposition.md`
- `ideas/open/716_prealloc_current_block_routing_authority_closure.md`
- `review/step6_2_owner_consumption_review.md`
- `src/backend/prealloc/publication_plans.cpp`
- `src/backend/prealloc/prepared_lookups.hpp`

## Current Targets

- `tests/backend/bir/backend_prealloc_join_transfer_destination_consistency_test.cpp`
- `tests/backend/bir/backend_prealloc_current_block_routed_operand_authority_test.cpp`
- `tests/backend/bir/backend_prealloc_current_block_all_edge_invariance_test.cpp`
- `tests/backend/mir/backend_aarch64_current_block_join_routing_test.cpp`

## Non-Goals

- Do not reopen accepted owner storage or lookup lifetime work.
- Do not grant Route 5, target reconstruction, result names, or successor-only
  identity authority.
- Do not rewrite supported integration vectors.
- Do not close ideas 716, 713, or 705 in this runbook.

## Execution Rules

- Establish a fresh registered baseline and inventory before semantic edits.
- Keep one primary authority contract per focused test.
- Prefer fail-closed semantic rules over instruction- or testcase-shaped
  matching.
- For code-changing steps, run build, the named focused test, and the exact
  supervisor-selected matching proof command.
- Run fresh broader backend proof before integration adoption and handback.

## Ordered Steps

### Step 1: Establish the blocked-family baseline and authority inventory

Goal: freeze the accepted owner boundary and map every rejected Step 6.2 fact
to one focused contract.

Actions:

- Record accepted HEAD `d253152e0` and the unchanged supported integration
  vectors as the starting contract.
- Inventory transfer result, edge destination, publication source, routed
  operand, immediate destination, predecessor, and parallel-edge facts.
- Confirm each proposed focused test is registered or identify the exact
  registration work required.
- Select a matching baseline/proof command without changing implementation or
  test expectations.

Completion check:

- Every rejected authority expansion maps to exactly one focused probe, the
  baseline is recorded, and no AArch64 behavior change has been attempted.

### Step 2: Prove join-transfer destination consistency

Goal: define the consistency rule between aggregate transfer result, selected
edge destination, and publication destination.

Primary target:
`tests/backend/bir/backend_prealloc_join_transfer_destination_consistency_test.cpp`

Actions:

- Prove matching and mismatching aggregate/edge/publication destinations.
- Fail closed on internal inconsistency unless construction itself proves the
  state impossible through a directly tested invariant.
- Keep routed operands and AArch64 consumption outside this probe.

Completion check:

- Destination consistency has one explicit, registered contract and focused
  build/proof is green.

### Step 3: Prove routed-operand and immediate-destination authority

Goal: define whether and how publication authority applies to scalar operands
and immediate-source destination homes.

Primary target:
`tests/backend/bir/backend_prealloc_current_block_routed_operand_authority_test.cpp`

Actions:

- Cover `BinaryInst`, `CastInst`, and `SelectInst` operands independently.
- Cover immediate-source destination authority, unrelated operands, and
  conflicting source semantics.
- Preserve prepared source identity; do not rewrite it merely to satisfy the
  stable query.

Completion check:

- Every supported routed-value category has a semantic rule and negative
  matrix independent of the integration fixture; focused proof is green.

### Step 4: Prove all-applicable-edge invariance

Goal: require agreement across the complete applicable fact family.

Primary target:
`tests/backend/bir/backend_prealloc_current_block_all_edge_invariance_test.cpp`

Actions:

- Cover distinct predecessors, destinations, sources, and semantic origins.
- Include parallel edges, missing facts, conflicting facts, and ambiguous
  duplicate families.
- Reject first-match, unique-subset, or successor-only selection.

Completion check:

- `Available` requires complete agreement across every applicable fact and the
  focused invariance matrix is green.

### Step 5: Compose the focused authority contracts

Goal: show the three rules coexist at the owner-attached query boundary.

Actions:

- Run all focused authority and owner-lifetime contracts together.
- Inspect the composed implementation for testcase-shaped rules or hidden
  source-identity rewriting.
- Run fresh broader backend proof before target consumption.

Completion check:

- Focused and broader proof are green and review finds no unresolved authority
  collision.

### Step 6: Adopt owner-only AArch64 consumption

Goal: consume the proven owner-attached stable-key query without reconstruction.

Primary target:
`tests/backend/mir/backend_aarch64_current_block_join_routing_test.cpp`

Actions:

- Remove target-local authority construction and fallbacks.
- Preserve every supported integration vector unchanged.
- Run focused contracts, integration proof, and fresh broader backend proof.

Completion check:

- AArch64 only consumes the owner query, supported vectors are unchanged, and
  focused, integration, and broader proof are green.

### Step 7: Hand back to idea 716

Goal: return the composed authority contract to the blocked closure initiative.

Actions:

- Record the durable completed contract and proof in idea 717.
- Switch lifecycle execution back to idea 716 at Step 6.2.
- Keep ideas 713 and 705 blocked pending idea 716 handback.

Completion check:

- Idea 717 is ready to close and idea 716 can resume with no unresolved
  routed-value authority seam.
