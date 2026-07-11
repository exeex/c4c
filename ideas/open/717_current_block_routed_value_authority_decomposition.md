# Current-Block Routed-Value Authority Decomposition

Status: Open
Type: backend authority decomposition
Blocks:
- `ideas/open/716_prealloc_current_block_routing_authority_closure.md`
- `ideas/open/713_current_block_edge_bound_routing_consumption_decomposition.md`
- `ideas/open/705_prepared_fact_boundary_from_bir_views.md`

## Goal

Decompose current-block routed-value authority into focused prepared-fact
contracts before owner-only AArch64 consumption resumes.

## Why This Idea Exists

Idea 716 Step 6.2 twice moved the first bad fact without establishing a safe,
general authority model. The accepted Step 6.1 owner storage at `d253152e0`
provides the right lifetime and query boundary, but the rejected consumption
route weakened `PreparedJoinTransfer` destination consistency, synthesized
authority for scalar operands and immediate destinations, did not prove
agreement across all applicable edge facts, and changed supported integration
vectors.

Those are distinct authority seams. Continuing inside the AArch64 integration
test would make one large fixture the discovery surface and encourage
testcase-shaped rules. Idea 716 therefore remains open and blocked while this
initiative establishes the missing contracts. Ideas 713 and 705 remain open
and blocked behind idea 716.

## In Scope

- Prove whether `PreparedJoinTransfer::result` and every authoritative edge
  destination must agree, and fail closed on internal inconsistency.
- Separate publication source identity from authority for scalar
  `BinaryInst`, `CastInst`, and `SelectInst` operands and for immediate-source
  destination homes.
- Require agreement across every applicable predecessor/destination fact,
  including parallel edges; missing, conflicting, and incomplete families
  remain non-authoritative.
- After the focused probes are green, adopt only the owner-attached stable-key
  query in AArch64 without reconstruction or expectation changes.

## Focused Registered Contracts

1. `tests/backend/bir/backend_prealloc_join_transfer_destination_consistency_test.cpp`
   owns aggregate-result versus edge/publication-destination consistency.
2. `tests/backend/bir/backend_prealloc_current_block_routed_operand_authority_test.cpp`
   owns publication-source, scalar-operand, and immediate-destination authority.
3. `tests/backend/bir/backend_prealloc_current_block_all_edge_invariance_test.cpp`
   owns agreement across every applicable predecessor/destination fact,
   including parallel edges.
4. `tests/backend/mir/backend_aarch64_current_block_join_routing_test.cpp`
   remains the unchanged integration contract and is not a discovery surface.

Each focused probe must have one primary contract. Equivalent existing
registered tests may be extended only when they already own exactly that seam.

## Out Of Scope

- Reopening owner attachment, lifetime, or owner storage already accepted in
  idea 716 Step 6.1.
- Route 5 authority, target-local reconstruction, function-wide scans, pointer
  identity, result-name matching, or successor-only authority.
- Rewriting supported AArch64 integration vectors or weakening supported cases
  to preserve a proposed authority rule.
- Broad publication-plan, BIR, MIR, or target refactors unrelated to the three
  focused authority seams.
- Closing ideas 716, 713, or 705 as part of this decomposition switch.

## Acceptance Criteria

- The destination-consistency probe proves that aggregate transfer result,
  selected edge destination, and publication destination cannot disagree and
  still authorize a routed value, unless a stronger construction invariant is
  proved directly.
- The routed-operand probe gives each scalar operand and immediate-destination
  category an explicit semantic rule, including unrelated and conflicting
  operands, without rewriting source identity into a tautological match.
- The all-edge probe proves agreement across every applicable fact and fails
  closed for missing, conflicting, duplicate-ambiguous, and parallel-edge
  destination/source families.
- Only after all three probes are green does AArch64 consume the owner-attached
  stable-key query; it adds no authority builder or fallback.
- Existing supported AArch64 integration vectors remain unchanged and fresh
  focused plus broader backend proof is green.
- The proven authority contract is precise enough to hand execution back to
  idea 716 Step 6.2 without unresolved routed-value authority questions.

## Reviewer Reject Signals

- A named integration vector, instruction instance, operand position, or
  immediate shape receives a special-case authority shortcut.
- `PreparedJoinTransfer::result` inconsistency is ignored without a focused
  proof that construction makes it impossible or safely irrelevant.
- Publication source identity is overwritten with the queried operand merely
  to make source-completeness checks pass.
- A unique, first, or agreeing subset of predecessor facts authorizes a value
  while another applicable or parallel edge is unexamined.
- Supported integration expectations are rewritten, downgraded, or marked
  unsupported without explicit user approval.
- Helper renames, expectation edits, classification-only changes, or green
  tests after contract weakening are claimed as capability progress.
- The rejected authority synthesis survives behind a new abstraction name, or
  broad unrelated publication/BIR/MIR/AArch64 rewriting replaces the focused
  contracts.

## Handback Criteria

Hand back to idea 716 only when all three focused authority contracts are
registered and green, their rules compose without contradiction, unchanged
AArch64 integration vectors are ready for owner-only consumption, and fresh
broader backend proof shows no regression. Keep ideas 713 and 705 blocked until
idea 716 completes its own handback.
