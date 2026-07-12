# Common Prepared Return-Chain Authority Runbook

Status: Active
Source Idea: ideas/open/727_common_prepared_return_chain_authority.md
Supersedes: the parked idea 709 Step 2.1 consumer route

## Purpose

Move prepared scalar return-chain semantics into typed common authority before
AArch64 consumer cleanup resumes.

## Goal

Publish a traversal-attached relation that completely identifies a prepared
return chain and fails closed when its evidence is unavailable or incoherent.

## Core Rule

Common preparation owns the semantic relation. Target materializers may
consume it later but must not participate in producing or completing it.

## Read First

- `ideas/open/727_common_prepared_return_chain_authority.md`
- `src/backend/mir/aarch64/codegen/alu.cpp` for the contract inventory only
- `src/backend/prealloc/prepared_object_traversal.cpp`
- `tests/backend/bir/backend_prepared_object_consumer_contract_test.cpp`

## Current Scope

- The return-chain facts presently reconstructed by
  `find_prepared_return_chain_facts`.
- Common prepared production, typed query/classification, traversal attachment,
  and focused common contract tests.

## Non-Goals

- Do not migrate or otherwise edit AArch64 consumers under this runbook.
- Do not change target ABI or instruction policy.
- Do not broaden into general scalar dataflow, scheduling, or regalloc work.
- Do not use testcase-shaped production, fixture-only authority, or weakened
  expectations.

## Working Model

- A valid relation authenticates the move chain, scalar producers, operand
  roles, terminal ABI-return home, first non-chain operand home, and freshness.
- Missing, stale, ambiguous, inconsistent, unsupported, or structurally
  incomplete evidence is typed fail-closed state.
- Traversal attachment is the consumer boundary; raw lookup rebuilding is not.

## Execution Rules

- Treat `alu.cpp` as evidence for the required contract, not as an owned edit.
- Start with a fact-by-fact inventory before selecting the common producer seam.
- Add the narrow typed API and producer path before any positive consumer proof.
- Prove at least two valid chain shapes plus nearby negative states.
- Stop for lifecycle review if the relation requires broad compiler dataflow or
  target ABI policy rather than a bounded prepared-MIR contract.

## Ordered Steps

### Step 1: Specify the common return-chain contract

Goal: turn the target reconstruction into a bounded, target-independent typed
contract.

Actions:

- Inventory every input, identity check, adjacency check, operand-role check,
  home lookup, and freshness decision in `find_prepared_return_chain_facts`.
- Map each input to an existing common prepared producer or document the first
  missing common fact.
- Define the typed result shape and precise fail-closed outcomes without
  copying target instruction policy into common code.
- Identify at least two valid chain shapes and the negative matrix needed to
  distinguish absent, stale, ambiguous, inconsistent, and unsupported state.

Completion check:

- The producer seam, query shape, identities, and negative outcomes are explicit
  enough to implement without consulting AArch64 policy or a named testcase.

### Step 2: Produce and attach typed return-chain authority

Goal: make common preparation publish the complete authenticated relation.

Primary targets: common prepared producer/query and traversal surfaces
identified in Step 1.

Actions:

- Add the smallest typed result and query surface that represents the complete
  relation.
- Produce it from authenticated common prepared facts, preserving move,
  producer, operand, home, ABI-return, and freshness identity.
- Attach the result to the appropriate traversal view.
- Keep every incomplete or contradictory state precise and fail closed.

Completion check:

- Traversal consumers can obtain the complete relation without rebuilding
  lookups, walking raw move bundles, or querying target-local scalar producers,
  and the project builds.

### Step 3: Prove the common contract

Goal: demonstrate general positive authority and precise failure behavior.

Primary target: `tests/backend/bir/backend_prepared_object_consumer_contract_test.cpp`

Actions:

- Add focused proof for at least two valid multi-instruction return-chain
  shapes.
- Cover missing, stale, ambiguous, inconsistent, unsupported, non-adjacent,
  wrong-chain-operand, and missing-home evidence as applicable to the contract.
- Run the supervisor-selected build and focused common prepared-query subset.
- Run the supervisor-selected broader backend checkpoint before milestone
  acceptance.

Completion check:

- Focused and broader proof are green without target synthesis, fixture-only
  production, expectation weakening, or retained reconstruction under a new
  name.

### Step 4: Hand authority back to idea 709

Goal: establish that the common initiative is complete and the parked AArch64
consumer route can resume.

Actions:

- Verify the traversal-attached API exposes every fact required to delete the
  AArch64 reconstruction and lookup fallback.
- Review the complete diff against the source idea's reject signals.
- Request lifecycle disposition; do not edit the AArch64 consumer in this step.

Completion check:

- The common contract is acceptance-ready and lifecycle review can reactivate
  idea 709 at its gated Step 2.1 consumer deletion.
