# BIR Select-Chain Direct-Global Identity Runbook

Status: Active
Source Idea: ideas/open/153_bir_select_chain_direct_global_identity.md

## Purpose

Move select-chain and direct-global dependency identity into target-neutral BIR
queries after the producer/source identity foundation, while keeping prepared
queries available as the comparison oracle.

## Goal

Add BIR-owned select-chain queries that match prepared answers for direct
global roots, select roots, scalar materialization, and negative cases before
any route-local consumer switch.

## Core Rule

Keep select-chain identity semantic and target-neutral. Do not import AArch64
routing policy, register availability, target materialization cost, call ABI
behavior, publication routing, or final move/branch choices into BIR.

## Read First

- `ideas/open/153_bir_select_chain_direct_global_identity.md`
- `docs/bir_prealloc_fusion/phase_a_normalization_candidates.md`
- existing prepared-query owners for:
  - `find_prepared_select_chain_source_producer`
  - `find_prepared_direct_global_select_chain_dependency`
  - `find_prepared_scalar_select_chain_materialization`
- BIR producer/source identity APIs added by
  `ideas/closed/152_bir_producer_source_identity_foundation.md`

## Current Targets

- Select-chain analysis keyed by block, root value, and before-instruction
  index.
- Root producer value/name, root-is-select, root instruction index, direct
  `LoadGlobalInst` dependency, and scalar materialization eligibility.
- Prepared/BIR query-equivalence proof for select root, non-select root, direct
  global load, and no-dependency paths.
- At most one route-local consumer switch after equivalence proof exists.

## Non-Goals

- Do not change target materialization cost, hazard decisions, register
  availability, publication routing policy, call ABI behavior, or final
  AArch64 move/branch choices.
- Do not rewrite broad MIR emission or prepared query ownership outside this
  route.
- Do not downgrade tests, weaken supported-path expectations, or claim progress
  through expectation rewrites.

## Working Model

- Prepared select-chain queries remain the oracle while BIR-owned queries are
  introduced.
- BIR select-chain identity should build on semantic producer/source identity
  rather than on target-specific publication, storage, or emission facts.
- Consumer switching is optional and must be narrow, route-local, and backed by
  prepared/BIR query-equivalence proof.

## Execution Rules

- Prefer small implementation packets with matching build/test proof.
- Use `todo.md` for packet progress, inspected evidence, proof commands, and
  closure notes.
- Preserve `test_before.log` and `test_after.log` as canonical matched logs
  when a packet changes code.
- Treat testcase-shaped select/global shortcuts, named-case-only fixes, and
  expectation downgrades as route failures.
- Keep source-idea edits unnecessary unless durable intent changes.

## Ordered Steps

### Step 1: Locate Prepared Select-Chain Surface And Evidence Set

Goal: establish current query owners, callers, data contracts, and proof cases
before adding BIR-owned select-chain state.

Primary target: prepared select-chain source producer, direct-global
dependency, and scalar materialization query code.

Actions:
- Locate definitions and direct callers of
  `find_prepared_select_chain_source_producer`,
  `find_prepared_direct_global_select_chain_dependency`, and
  `find_prepared_scalar_select_chain_materialization`.
- Identify the prepared facts each query consumes and returns.
- Identify representative select root, non-select root, direct global load,
  scalar materialization, and no-dependency cases for equivalence proof.
- Record the narrow backend/codegen subset the executor should use for the
  first implementation packet.

Completion check:
- `todo.md` names inspected files/functions, callers, candidate proof cases,
  and the proposed first code-changing packet.

### Step 2: Define BIR Select-Chain Identity Records

Goal: add target-neutral BIR record shapes for select-chain and direct-global
dependency identity.

Primary target: BIR identity/query types near the existing MIR query surface.

Actions:
- Define request and result records keyed by block, root value, and
  before-instruction index.
- Represent root producer value/name, root-is-select, root instruction index,
  direct `LoadGlobalInst` dependency, and scalar materialization eligibility.
- Build on existing BIR same-block producer/source identity where possible.
- Exclude target materialization cost, register state, publication routing,
  call ABI state, and final emission choices.

Completion check:
- The new BIR vocabulary compiles and does not import rejected target-specific
  or publication-owned authority.

### Step 3: Add BIR-Owned Select-Chain Queries

Goal: expose BIR-owned queries that answer the same semantic subset as the
prepared select-chain query surface.

Primary target: per-function analysis or MIR query implementation.

Actions:
- Populate select-chain identity from BIR instructions and same-block
  producer/source facts.
- Add query APIs for select-chain source producer, direct-global dependency,
  and scalar materialization eligibility.
- Keep the existing prepared queries available as comparison oracles.
- Avoid consumer rewrites in this step except for test-only comparison hooks.

Completion check:
- BIR select-chain queries compile and can be compared against prepared query
  results without changing broad backend behavior.

### Step 4: Prove Prepared/BIR Select-Chain Equivalence

Goal: demonstrate that BIR select-chain queries match the prepared oracle for
accepted positive and negative paths.

Primary target: targeted backend/codegen tests, assertions, or dumps.

Actions:
- Add or update proof for select root, non-select root, direct global load,
  scalar materialization, and no-dependency paths.
- Include negative coverage for before-boundary, missing dependency, and type
  or root mismatches where the existing prepared semantics reject.
- Capture matched `test_before.log` and `test_after.log` for the smallest
  backend/codegen subset that exercises select-chain dependency routing.
- Investigate mismatches as semantic gaps instead of special-casing named
  failing cases.

Completion check:
- Matched proof logs show no regression, and equivalence evidence covers each
  select-chain path named by the source idea.

### Step 5: Switch One Route-Local Consumer If Proven

Goal: move one select-materialization or direct-global dependency consumer to
the BIR query only after equivalence proof exists.

Primary target: one narrow consumer that only reads already-proven select-chain
identity facts.

Actions:
- Select a consumer with minimal blast radius and no ownership of register,
  storage, call ABI, or final emission policy.
- Switch only identity reads already proven equivalent.
- Keep prepared queries available for comparison and fallback during this
  route.
- Run the delegated narrow subset and escalate validation if shared codegen
  behavior changes.

Completion check:
- One route-local consumer uses BIR select-chain identity for proven facts,
  matched proof remains green, and no broad emission rewrite is included.

### Step 6: Acceptance Review And Close Payload

Goal: make closure evidence explicit for supervisor and reviewer handoff.

Actions:
- Summarize the BIR select-chain vocabulary, query APIs, equivalence proof
  cases, and any consumer switch in `todo.md`.
- Confirm the prepared query oracle remains available.
- Confirm no rejected target-specific state entered the BIR relationship.
- Confirm matched regression logs exist for the final code-changing scope.

Completion check:
- `todo.md` contains close-ready evidence and proof commands needed for
  lifecycle closure review.
