# Common Prepared Return-Chain Production Authority Repair Runbook

Status: Active
Source Idea: ideas/open/727_common_prepared_return_chain_authority.md
Supersedes: the parked idea 709 Step 2.1 consumer attempt

## Purpose

Repair the gap between the typed common return-chain classification and the
proof-attribution/freshness authority carried by real producer inputs.

## Goal

Make representative production return chains publish consumable, authenticated
`Available` relations without target assistance or fail-open behavior.

## Core Rule

Common production owns proof attribution and freshness. Do not rebuild the
relation in AArch64, weaken `Stale`, or inject authority only in a named fixture.

## Read First

- `ideas/open/727_common_prepared_return_chain_authority.md`
- `ideas/open/709_aarch64_named_handoff_materializer_cleanup.md`
- `test_after.log`
- the common prepared producer, classification, traversal, and fixture builders
  used by the failing return-chain inputs

## Current Scope

- Production and test-builder paths that construct prepared return-chain move
  bundles and their proof-attribution/freshness evidence.
- The existing typed `PreparedObjectReturnChainClassification` contract.
- Common contract proof plus representative AArch64 return-chain integration
  proof while the AArch64 consumer remains unchanged.

## Non-Goals

- No AArch64 consumer migration or helper deletion.
- No target-side semantic reconstruction or ABI-policy movement.
- No status relabeling, fail-open handling, expectation weakening, or
  testcase-shaped authority injection.
- No redesign outside the bounded prepared return-chain producer seam.

## Working Model

- The typed relation shape exists, but availability depends on authenticated
  move-bundle proof attribution and freshness.
- Synthetic classification coverage is necessary but insufficient for handoff.
- Real consumer inputs must carry the same authority through their normal
  common production path before idea 709 can resume.

## Execution Rules

- Preserve `Stale` for genuinely missing or mismatched authority.
- Find the earliest common owner of the missing attribution/freshness evidence.
- Generalize across at least two return-chain shapes; do not key production to
  AArch64 test names, opcode sequences, or fixed positions.
- Keep AArch64 implementation files unchanged during this runbook.
- Use matching before/after proof and retain the known instruction-dispatch
  baseline separately from return-chain regressions.

## Ordered Steps

### Step 1: Audit the stale production inputs

Goal: identify the first missing or mismatched common authority fact on real
return-chain inputs.

Actions:

- Trace representative one-link and multi-link return chains from their normal
  prepared builders through traversal attachment and classification.
- Compare their move-bundle proof attribution and freshness identities with an
  `Available` common-contract case.
- Record the earliest common producer/fixture owner and distinguish absent,
  stale, mismatched, and structurally unsupported evidence.
- Confirm the repair can remain target-independent and bounded to idea 727.

Completion check:

- The first bad authority fact and owning common seam are explicit, with at
  least two representative shapes and nearby negative states identified.

### Step 2: Publish complete attributed and fresh authority

Goal: repair the common production path so valid return chains can classify
`Available`.

Actions:

- Populate the missing proof-attribution/freshness evidence at its earliest
  common owner.
- Preserve instruction, value, move, operand-role, home, and freshness identity
  across the relation.
- Keep missing, stale, ambiguous, inconsistent, unsupported, and non-adjacent
  inputs typed and fail closed.
- Avoid target conditionals and fixture-only injection.

Completion check:

- Common traversal attaches an authenticated `Available` relation for both
  representative valid shapes while negative states retain precise outcomes,
  and the project builds.

### Step 3: Prove production-to-consumer readiness

Goal: demonstrate that common authority is consumable by the inputs that
blocked idea 709 without editing the AArch64 consumer.

Actions:

- Extend common producer/query proof for both valid shapes and the implicated
  attribution/freshness negatives.
- Add or use a read-only integration probe that observes the attached
  classification on representative AArch64 return-chain inputs.
- Run the supervisor-selected matching focused before/after command, including
  the return-lowering and external add/sub-chain surfaces.
- Run the broader backend checkpoint required for milestone acceptance.

Completion check:

- Common and integration proof show authenticated `Available` relations on
  real inputs, no new failures beyond the recorded baseline, and no target
  synthesis or expectation weakening.

### Step 4: Re-audit the handoff to idea 709

Goal: decide whether the parked consumer deletion can safely resume.

Actions:

- Verify every fact required by Step 2.1 is traversal-attached and fresh on the
  production inputs.
- Review the complete diff against idea 727's reject signals.
- Request lifecycle disposition; do not edit AArch64 consumers in this step.

Completion check:

- The common producer contract is acceptance-ready and lifecycle review has
  evidence, not assumption, for whether idea 709 may resume.
