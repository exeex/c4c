# Route 3 Semantic Memory Access Cache Split

Status: Active
Source Idea: ideas/open/169_route3_semantic_memory_access_cache_split.md

## Purpose

Turn the Route 3 follow-up into bounded execution: migrate one selected
semantic memory/access consumer group from prepared memory lookup helpers to
Route 3 BIR memory/access records, then contract only prepared semantic
cache/API surface that is no longer publicly consumed.

## Goal

Make one selected consumer read `Route3MemoryAccessIndex` or a thin
BIR-backed facade for semantic access/source identity while prepared answers
remain available as migration oracles until positive and negative equivalence
is proven.

## Core Rule

Do not copy `PreparedAddress`, `PreparedMemoryAccess`, frame-slot ids, byte
offsets, relocation/TLS details, base-plus-offset legality, or AArch64 memory
operand formation into BIR. Route 3 owns semantic memory/source identity only;
target addressing, layout, and emission policy stay prepared/prealloc or
target-owned.

## Read First

- `ideas/open/169_route3_semantic_memory_access_cache_split.md`
- `docs/bir_prealloc_fusion/phase_c_private_cache_contraction.md`
- `ideas/closed/154_bir_memory_access_identity.md`
- `ideas/closed/161_bir_memory_access_identity_annotation_schema.md`
- `src/backend/mir/query.hpp`
- `src/backend/prealloc/addressing.hpp`
- `src/backend/prealloc/prepared_lookups.cpp`
- Current memory, call, global, and store-retargeting consumers of prepared
  memory/access answers

## Scope

- Direct memory access identity.
- Same-block global-load access.
- Load-local stored-value source facts.
- Result/stored-value identity, address-space/volatile bits, semantic base
  kind, and local/global/pointer/string source identity.
- One semantic consumer family from memory, call, global, or store-retargeting
  paths.

## Non-Goals

- Do not migrate `PreparedAddress`, `PreparedMemoryAccess`, or
  `PreparedAddressMaterializationLookups` wholesale.
- Do not change frame slots, byte offsets, size/align layout, relocation/TLS
  records, stack objects, base-plus-offset legality, or memory operand
  formation.
- Do not replace indexed Route 3 access with broad same-block scans.
- Do not claim progress by weakening tests, rewriting expectations, or adding
  named-case shortcuts.

## Working Model

- Route 3 owns target-neutral memory/access facts through
  `Route3MemoryAccessIndex`, `Route3MemoryAccessRecord`,
  `Route3MemoryAccessValueRecord`, `Route3SameBlockGlobalLoadAccessRecord`,
  and `Route3SameBlockLoadLocalSourceRecord`.
- Accepted facts include direct access identity, same-block global-load
  identity, load-local stored-value source links, access kind, semantic base
  kind, result/stored-value roles, address-space, and volatile bits.
- Prepared lookup answers are the comparison oracle during migration.
- Public aggregate/cache contraction is allowed only for semantic identity
  fields whose selected consumer has moved and whose remaining public uses have
  been audited.
- Target/layout facts remain out of scope even when they live beside semantic
  lookup facts in the same prepared record family.

## Execution Rules

- Pick exactly one semantic consumer family for the first packet.
- Prefer Route 3 query helpers over testcase-shaped matching or manual BIR
  rescans.
- Keep any compatibility facade narrow and BIR-backed.
- Leave unrelated prepared address/materialization, target lowering policy,
  stack layout, ABI, and higher-route consumers alone.
- For code-changing steps, prove with build or compile proof first, then the
  narrow backend subset named by the executor packet.
- Escalate validation if public memory/addressing headers, aggregate
  projections, memory operand formation, call ABI behavior, or store
  retargeting behavior change.

## Ordered Steps

### Step 1: Inventory Candidate Route 3 Consumers

Goal: Identify one bounded semantic memory/access consumer family that can
migrate without target-addressing or layout changes.

Primary targets:
- `Route3MemoryAccessIndex` and Route 3 MIR query helpers
- `src/backend/mir/query.hpp`
- `src/backend/prealloc/addressing.hpp`
- `src/backend/prealloc/prepared_lookups.cpp`
- Memory, call, global, and store-retargeting consumers of prepared
  memory/access answers
- `PreparedFunctionLookups` fields that expose memory/access lookup caches

Actions:
- Inspect direct consumers of prepared Route 3-adjacent lookup fields.
- Classify each candidate as semantic Route 3, target-addressing-bound,
  layout/ABI-bound, higher-route oracle, or aggregate-only exposure.
- Select one consumer family with nearby positive and negative coverage.
- Record rejected candidates and why they remain out of scope in `todo.md`.

Completion check:
- `todo.md` names the selected consumer family, target files, proof command,
  and explicit positive and negative cases before implementation begins.

### Step 2: Add Or Confirm Route 3 Oracle Coverage

Goal: Ensure the selected consumer can compare BIR and prepared answers before
the consumer switch.

Primary targets:
- Route 3 memory/access tests
- Backend prepared memory operand or prepared lookup helper tests
- Existing narrow target-lowering subset for the selected consumer

Actions:
- Add or extend oracle tests for the selected consumer's Route 3 facts.
- Cover direct memory access or same-block access/source positive cases.
- Cover fail-closed rejects for wrong node kind, mismatched root/source,
  missing access, unsupported source shape, and out-of-order same-block use as
  relevant to the selected consumer.
- Keep prepared helpers visible as expected oracle sources.

Completion check:
- Focused tests prove BIR/prepared equivalence for the selected consumer
  family, including required positive and reject cases.

### Step 3: Migrate The Selected Consumer

Goal: Move the selected consumer to `Route3MemoryAccessIndex` or a thin
BIR-backed facade without changing target addressing/layout policy.

Primary targets:
- The selected consumer files from Step 1
- Route 3 query/facade helpers if a narrow adapter is needed

Actions:
- Replace direct prepared lookup reads for selected semantic facts with Route 3
  query reads.
- Keep prepared answers only as oracle, compatibility fallback, or
  target/layout source where still required.
- Avoid broad scans and avoid duplicating prepared addressing payloads in BIR.
- Preserve behavior for missing access, wrong node kind, unsupported source,
  and out-of-order same-block cases.

Completion check:
- The selected consumer no longer reads the prepared Route 3-adjacent semantic
  field as its primary semantic source, and the narrow proof subset is green.

### Step 4: Contract Only Proven-Private Surface

Goal: Remove or narrow only the prepared semantic memory/access cache/API
exposure that no longer has public consumers after the selected migration.

Primary targets:
- `PreparedFunctionLookups` projected fields for the migrated semantic group
- Public includes or context projections made unused by the migration
- Prepared same-block global-load or load-local source helper declarations made
  unused by the selected migration

Actions:
- Re-scan direct consumers before deleting or hiding any field/helper.
- Keep compatibility surfaces public if memory, call, global,
  store-retargeting, AArch64, x86, or RISC-V consumers still require them.
- Keep `PreparedAddress`, `PreparedMemoryAccess`, address materialization, and
  stack-layout surfaces public when target/layout consumers remain.
- Limit include/context contraction to behavior-preserving changes.

Completion check:
- Any contraction is backed by consumer evidence and compile proof; no prepared
  helper needed by remaining public consumers is hidden.

### Step 5: Validate And Handoff

Goal: Prove the migration and leave precise lifecycle state for the next
packet.

Actions:
- Run the delegated narrow proof command for the selected consumer.
- Run broader backend validation if public headers, aggregate projections,
  memory operand formation, store retargeting, or call ABI behavior changed.
- Update `todo.md` with proof results, residual consumers, and the next
  bounded Route 3 candidate if one remains.

Completion check:
- Build/proof logs are fresh, `todo.md` reflects the current state, and the
  remaining prepared memory/access surfaces are either still public by evidence
  or contracted by a proven consumer migration.
