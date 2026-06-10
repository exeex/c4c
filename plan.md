# BIR Memory Access Identity Runbook

Status: Active
Source Idea: ideas/open/154_bir_memory_access_identity.md

## Purpose

Move semantic memory/access identity for loads, stores, address sources, and
stored-value recovery into BIR-owned queries while keeping frame layout, target
addressing, relocation, and AArch64 operand formation downstream.

## Goal

Add BIR memory-access queries that match prepared answers for
BIR-representable semantic identity cases for local, global, pointer, string,
volatile, same-block global-load, and load-local source cases before switching
any consumer reads.

## Core Rule

Keep BIR memory identity semantic. Do not import prepared frame slot ids,
concrete offsets, size/align layout, base-plus-offset legality, TLS relocation
spelling, GOT/direct/page-low policy, target addressing-mode choice, or AArch64
memory operand legality.

## Read First

- `ideas/open/154_bir_memory_access_identity.md`
- `docs/bir_prealloc_fusion/phase_a_normalization_candidates.md`
- `src/backend/prealloc/addressing.hpp`
- `src/backend/prealloc/prepared_lookups.cpp`
- `src/backend/prealloc/publication_plans.hpp`
- `src/backend/prealloc/publication_plans.cpp`
- existing BIR producer/source identity APIs added by the recent foundation
  and select-chain routes

## Current Targets

- BIR memory-access identity attached to `LoadLocalInst`, `LoadGlobalInst`,
  `StoreLocalInst`, `StoreGlobalInst`, and address-materialization nodes where
  the source idea requires it.
- Result/stored value names, address space, volatile flag, semantic base kind,
  pointer/global/local/string source identity, same-block global-load access,
  and load-local stored-value source links.
- Prepared/BIR query-equivalence proof for accepted memory access and source
  recovery paths within the BIR-representable semantic domain before any
  consumer switch.
- Prepared-only positives that require stack-layout range overlap or
  non-overlap math remain oracle/fallback evidence, not BIR parity
  requirements.
- At most one route-local consumer switch after equivalence proof exists.

## Non-Goals

- Do not move frame slot ids, byte offsets, concrete size/align, stack layout,
  TLS register or relocation spelling, GOT/direct/page-low choice,
  base-plus-offset legality, target addressing-mode choice, or AArch64 memory
  operand formation into BIR.
- Do not rewrite broad MIR memory emission, stack-layout ownership,
  publication storage policy, or prepared address materialization policy.
- Do not downgrade tests, weaken supported-path expectations, or claim
  progress through expectation rewrites.

## Working Model

- Prepared memory/access queries remain the oracle while BIR-owned queries are
  introduced.
- BIR uses stable semantic names and BIR slot/name identity for locals instead
  of prepared frame slot ids.
- BIR equivalence means parity for facts BIR can represent semantically:
  operation identity, source identity, result/stored names, address space,
  volatile flag, semantic base kind, and same-block producer/source links.
- Prepared answers that depend on frame offsets, concrete sizes, alignment, or
  stack-layout range overlap/non-overlap are outside the BIR equivalence
  domain and remain prepared-owned fallback/oracle behavior.
- Store-source and memory-retargeting consumers may switch only for semantic
  source identity after prepared/BIR equivalence is proven.
- Target addressing and layout consumers continue to read prepared or
  downstream target-owned facts.

## Execution Rules

- Prefer small implementation packets with matching build/test proof.
- Use `todo.md` for packet progress, inspected evidence, proof commands, and
  closure notes.
- Preserve `test_before.log` and `test_after.log` as canonical matched logs
  when a packet changes code.
- Treat testcase-shaped memory shortcuts, named-case-only fixes, target-policy
  leakage into BIR, and expectation downgrades as route failures.
- Escalate to broader backend validation if address materialization, stack
  layout, or AArch64 memory operand code changes.
- Keep source-idea edits unnecessary unless durable intent changes.

## Ordered Steps

### Step 1: Locate Prepared Memory Surface And Evidence Set

Goal: establish current query owners, callers, data contracts, and proof cases
before adding BIR-owned memory state.

Primary target: prepared memory/access, same-block global-load, load-local
stored-value, and store-source publication query code.

Actions:
- Locate definitions and direct callers of `PreparedAddress`,
  `PreparedMemoryAccess`, `PreparedSameBlockGlobalLoadAccess`,
  `PreparedSameBlockLoadLocalStoredValueSource`,
  `find_prepared_same_block_global_load_access`,
  `find_prepared_same_block_load_local_stored_value_source`, and
  `find_prepared_same_block_load_local_source_producer`.
- Identify the semantic fields each query consumes and returns separately from
  rejected target/layout fields.
- Identify representative local, global, pointer, string, volatile,
  same-block global-load, load-local stored-value, and unavailable cases for
  equivalence proof.
- Record the narrow backend/codegen subset the executor should use for the
  first implementation packet.

Completion check:
- `todo.md` names inspected files/functions, callers, candidate proof cases,
  rejected fields, and the proposed first code-changing packet.

### Step 2: Define BIR Memory Access Identity Records

Goal: add target-neutral BIR record shapes for memory operation identity and
semantic access sources.

Primary target: BIR identity/query types near the existing producer/source
relationship surface.

Actions:
- Define request and result records keyed by stable BIR operation identity,
  block label, value/name, and before-instruction index where needed.
- Represent result value, stored value, address space, volatile flag, semantic
  base kind, pointer/global/local/string source identity, same-block
  global-load access, and load-local stored-value source links.
- Use BIR slot/name identity for locals instead of prepared frame slot ids.
- Exclude layout, relocation, target addressing legality, destination homes,
  storage encoding, and AArch64 operand formation.

Completion check:
- The new BIR vocabulary compiles and does not import rejected prepared
  address, stack-layout, publication-storage, or target-addressing authority.

### Step 3: Add BIR-Owned Memory Access Queries

Goal: expose BIR-owned queries for the semantic subset of the prepared
memory/access query surface that BIR can represent without layout authority.

Primary target: per-function BIR analysis or MIR query implementation.

Actions:
- Populate memory identity from BIR load, store, address, local, global,
  pointer, and string facts.
- Add query APIs for memory access identity, same-block global-load access,
  and load-local stored-value source recovery.
- Build on existing producer/source identity where load-local source recovery
  needs a prior semantic producer.
- For load-local source recovery, fail closed on intervening same-slot stores
  unless BIR owns a semantic non-overlap fact that is not derived from stack
  layout.
- Treat prepared positives that require stack-layout range non-overlap as
  oracle-only evidence; do not copy frame offsets, concrete sizes, alignment,
  or overlap math into BIR.
- Keep the existing prepared queries available as comparison oracles.
- Avoid consumer rewrites in this step except for test-only comparison hooks.

Completion check:
- BIR memory queries compile, expose only semantic memory/source facts, and
  can be compared against prepared query results without changing broad
  backend behavior. Known prepared-only layout positives are documented as
  fail-closed BIR boundaries, not parity failures.

### Step 4: Prove BIR-Representable Memory Equivalence

Goal: demonstrate that BIR memory/access queries match the prepared oracle for
accepted positive and negative paths inside the BIR-representable semantic
domain.

Primary target: targeted backend/codegen tests, assertions, or dumps.

Actions:
- Add or update proof for local, global, pointer, string, volatile,
  same-block global-load, load-local stored-value, store-source, and
  unavailable paths when the answer depends only on BIR-owned semantic facts.
- Include negative coverage for source mismatch, missing access, missing prior
  store/source, and rejected target-only address facts where the existing
  prepared semantics reject.
- Include explicit boundary coverage for prepared-only positives that depend
  on stack-layout range non-overlap, proving they remain prepared
  oracle/fallback behavior while BIR fails closed.
- Capture matched `test_before.log` and `test_after.log` for the smallest
  backend/codegen subset that exercises memory/access source identity.
- Investigate mismatches as semantic gaps instead of special-casing named
  failing cases.
- Do not treat prepared acceptance that depends on frame offsets, concrete
  sizes, alignment, or range overlap/non-overlap as a BIR equivalence failure.

Completion check:
- Matched proof logs show no regression; equivalence evidence covers each
  memory/access path named by the source idea within the BIR-representable
  semantic domain; and prepared-only layout positives are preserved as
  documented oracle/fallback cases.

### Step 5: Switch One Semantic Memory Consumer If Proven

Goal: move one store-source, memory-retargeting, or source-recovery consumer to
the BIR query only after BIR-representable equivalence proof exists.

Primary target: one narrow consumer that only reads already-proven semantic
memory/source identity facts.

Actions:
- Select a consumer with minimal blast radius and no ownership of layout,
  relocation, target addressing, storage encoding, or AArch64 operand policy.
- Switch only identity reads already proven equivalent.
- Do not switch load-local source paths whose correctness depends on prepared
  stack-layout range overlap/non-overlap proof.
- Keep prepared queries available for comparison and fallback during this
  route.
- Run the delegated narrow subset and escalate validation if shared backend
  memory behavior changes.

Completion check:
- One route-local consumer uses BIR memory identity only for proven
  BIR-representable semantic facts, prepared remains available for
  layout-dependent fallback, matched proof remains green, and no target
  addressing or layout rewrite is included.

### Step 6: Acceptance Review And Close Payload

Goal: make closure evidence explicit for supervisor and reviewer handoff.

Actions:
- Summarize the BIR memory vocabulary, query APIs, equivalence proof cases,
  and any consumer switch in `todo.md`.
- Confirm the prepared query oracle remains available.
- Confirm no rejected target/layout/addressing state entered the BIR
  relationship.
- Confirm matched regression logs exist for the final code-changing scope.

Completion check:
- `todo.md` contains close-ready evidence and proof commands needed for
  lifecycle closure review.
