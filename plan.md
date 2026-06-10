# Route 2 Select-Chain Direct-Global Oracle Thinning

Status: Active
Source Idea: ideas/open/168_route2_select_chain_direct_global_oracle_thinning.md

## Purpose

Turn the Route 2 follow-up into bounded execution: migrate one selected
select-chain/direct-global consumer group from prepared lookup helpers to
Route 2 BIR records, then contract only prepared cache/API surface that is no
longer needed by that consumer.

## Goal

Make one selected consumer read `Route2SelectChainValueIndex` or a thin
BIR-backed facade while prepared answers remain available as migration oracles
until positive and negative equivalence is proven.

## Core Rule

Do not hide or delete prepared select-chain, direct-global, or scalar
materialization helpers until the selected consumer no longer depends on them
except as an oracle or fallback, and do not move materialization cost, hazard
decisions, register availability, publication routing, call ABI behavior, or
final move/branch choices into BIR.

## Read First

- `ideas/open/168_route2_select_chain_direct_global_oracle_thinning.md`
- `docs/bir_prealloc_fusion/phase_c_private_cache_contraction.md`
- `ideas/closed/160_bir_select_chain_global_dependency_annotation_schema.md`
- `src/backend/prealloc/select_chain_lookups.hpp`
- Route 2 BIR select-chain records and query helpers
- Current consumers of prepared select-chain, direct-global, and scalar
  materialization lookup fields

## Scope

- Select-chain value/dependency lookup caches.
- Scalar materialization eligibility lookup.
- Direct-global dependency indexes layered through select-chain/source-producer
  lookup fields.
- One consumer family from call, publication, materialization, or
  source-producer paths.

## Non-Goals

- Do not change materialization cost, hazard decisions, register availability,
  publication routing, call ABI behavior, or final move/branch choices.
- Do not delete prepared select-chain helpers while unaudited consumers still
  depend on them.
- Do not turn select-chain identity into a call-specific or AArch64-specific
  shortcut.
- Do not claim progress by weakening tests, rewriting expectations, or adding
  named-case shortcuts.

## Working Model

- Route 2 owns target-neutral select-chain/direct-global facts through
  `Route2SelectChainValueIndex` and associated query helpers.
- Accepted facts include select roots, direct `LoadGlobalInst` dependencies,
  root producers, root instruction indexes, and scalar materialization
  eligibility.
- Prepared lookup answers are the comparison oracle during migration.
- Public aggregate/cache contraction is allowed only for fields whose selected
  consumer has moved and whose remaining public uses have been audited.
- Negative cases are first-class proof requirements: direct-global,
  non-select-root, no-dependency, and scalar-ineligible cases must stay
  fail-closed.

## Execution Rules

- Pick exactly one consumer family for the first packet.
- Prefer semantic Route 2 query use over testcase-shaped matching.
- Keep any compatibility facade narrow and BIR-backed.
- Leave unrelated prepared lookup families, target lowering policy, and higher
  route consumers alone.
- For code-changing steps, prove with build or compile proof first, then the
  narrow backend subset named by the executor packet.
- Escalate validation if public headers, aggregate projections, call ABI,
  publication routing, or materialization lowering behavior change.

## Ordered Steps

### Step 1: Inventory Candidate Route 2 Consumers

Goal: Identify one bounded select-chain/direct-global consumer family that can
migrate without broad target-policy changes.

Primary targets:
- `Route2SelectChainValueIndex` and Route 2 MIR query helpers
- `src/backend/prealloc/select_chain_lookups.hpp`
- `src/backend/prealloc/select_chain_lookups.cpp`
- Call, publication, scalar materialization, and source-producer consumers of
  prepared select-chain/direct-global answers
- `PreparedFunctionLookups` fields that expose select-chain/source-producer
  caches

Actions:
- Inspect direct consumers of prepared Route 2-adjacent lookup fields.
- Classify each candidate as semantic Route 2, target-policy-bound,
  higher-route oracle, or aggregate-only exposure.
- Select one consumer family with nearby positive and negative coverage.
- Record rejected candidates and why they remain out of scope in `todo.md`.

Completion check:
- `todo.md` names the selected consumer family, target files, proof command,
  and explicit positive and negative cases before implementation begins.

### Step 2: Add Or Confirm Route 2 Oracle Coverage

Goal: Ensure the selected consumer can compare BIR and prepared answers before
the consumer switch.

Primary targets:
- Route 2 select-chain/direct-global tests
- Backend prepared lookup helper tests
- Existing narrow target-lowering subset for the selected consumer

Actions:
- Add or extend oracle tests for the selected consumer's Route 2 facts.
- Cover direct-global positive cases.
- Cover fail-closed rejects for non-select-root, no-dependency, and
  scalar-ineligible values.
- Keep prepared helpers visible as expected oracle sources.

Completion check:
- Focused tests prove BIR/prepared equivalence for the selected consumer
  family, including direct-global positive and required reject cases.

### Step 3: Migrate The Selected Consumer

Goal: Move the selected consumer to `Route2SelectChainValueIndex` or a thin
BIR-backed facade without changing target lowering policy.

Primary targets:
- The selected consumer files from Step 1
- Route 2 query/facade helpers if a narrow adapter is needed

Actions:
- Replace direct prepared lookup reads for the selected semantic facts with
  Route 2 query reads.
- Keep prepared answers only as oracle or compatibility fallback where still
  required.
- Avoid broad scans and avoid duplicating target-owned state in BIR.
- Preserve behavior for unsupported, non-select-root, no-dependency, and
  scalar-ineligible cases.

Completion check:
- The selected consumer no longer reads the prepared Route 2-adjacent field as
  its primary semantic source, and the narrow proof subset is green.

### Step 4: Contract Only Proven-Private Surface

Goal: Remove or narrow only the prepared cache/API exposure that no longer has
public consumers after the selected migration.

Primary targets:
- `PreparedFunctionLookups` projected fields for the migrated group
- Public includes or context projections made unused by the migration
- Prepared select-chain/source-producer helper declarations made unused by the
  selected migration

Actions:
- Re-scan direct consumers before deleting or hiding any field/helper.
- Keep compatibility surfaces public if call, publication, materialization,
  source-producer, AArch64, x86, or RISC-V consumers still require them.
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
  materialization lowering, publication routing, or call ABI behavior changed.
- Update `todo.md` with proof results, residual consumers, and the next bounded
  Route 2 candidate if one remains.

Completion check:
- Build/proof logs are fresh, `todo.md` reflects the current state, and the
  remaining prepared lookup surfaces are either still public by evidence or
  contracted by a proven consumer migration.
