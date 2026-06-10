# Route 1 Producer And Constant Oracle Thinning

Status: Active
Source Idea: ideas/open/167_route1_producer_constant_oracle_thinning.md

## Purpose

Turn the Route 1 follow-up into bounded execution: migrate one producer,
constant, or value-materialization consumer group from prepared same-block
lookup helpers to Route 1 BIR producer records, then contract only cache/API
surface that no longer has public consumers.

## Goal

Make one selected consumer read `Route1ProducerIndex` or a thin BIR-backed
facade while prepared answers remain available as migration oracles until
equivalence is proven.

## Core Rule

Do not hide or delete prepared producer, constant, or materialization helpers
until the selected consumer no longer depends on them except as an oracle or
fallback, and do not import target storage or final lowering policy into BIR.

## Read First

- `ideas/open/167_route1_producer_constant_oracle_thinning.md`
- `docs/bir_prealloc_fusion/phase_c_private_cache_contraction.md`
- `src/backend/prealloc/prepared_lookups.hpp`
- Route 1 BIR producer/index definitions and query helpers
- Current consumers of same-block producer, integer constant, and
  materialization lookup fields

## Scope

- Same-block scalar producer queries.
- Immediate integer constant queries.
- Same-block value-materialization query/cache use.
- Source-producer lookup cache exposure through `PreparedFunctionLookups`.
- One bounded consumer class at a time.

## Non-Goals

- Do not move homes, registers, storage, emitted-register state, operand views,
  frame slots, final instruction order, spill/reload behavior, or publication
  hooks into BIR.
- Do not delete prepared producer or constant helpers before the selected
  consumer no longer needs them as an oracle.
- Do not perform broad AArch64 materialization rewrites.
- Do not claim progress by weakening tests, rewriting expectations, or adding
  named-case shortcuts.

## Working Model

- Route 1 owns target-neutral producer and constant facts through
  `Route1ProducerIndex` and associated query helpers.
- Prepared lookup answers are the comparison oracle during migration.
- Public aggregate/cache contraction is allowed only for fields whose selected
  consumer has moved and whose remaining public uses have been audited.
- Negative cases are first-class proof requirements: no producer, no constant,
  and non-materializable values must stay fail-closed.

## Execution Rules

- Pick exactly one consumer family for the first packet.
- Prefer semantic Route 1 query use over testcase-shaped matching.
- Keep any compatibility facade narrow and BIR-backed.
- Leave unrelated prepared lookup families, target lowering policy, and higher
  route consumers alone.
- For code-changing steps, prove with build or compile proof first, then the
  narrow backend subset named by the executor packet.
- Escalate validation if public headers, aggregate projections, or target
  lowering behavior change.

## Ordered Steps

### Step 1: Inventory Candidate Route 1 Consumers

Goal: Identify one bounded producer, constant, or materialization consumer
family that can migrate without broad target-policy changes.

Primary targets:
- `PreparedFunctionLookups` producer/constant/materialization fields
- Same-block scalar producer and immediate integer constant query consumers
- Source-producer lookup paths that can read Route 1 facts directly

Actions:
- Inspect direct consumers of prepared Route 1-adjacent lookup fields.
- Classify each candidate as semantic Route 1, target-policy-bound, higher-route
  oracle, or aggregate-only exposure.
- Select one consumer family with nearby positive and negative coverage.
- Record rejected candidates and why they remain out of scope in `todo.md`.

Completion check:
- `todo.md` names the selected consumer family, target files, proof command, and
  explicit negative cases before implementation begins.

### Step 2: Add Or Confirm Route 1 Oracle Coverage

Goal: Ensure the selected consumer can compare BIR and prepared answers before
the consumer switch.

Primary targets:
- Route 1 producer/index tests
- Backend prepared lookup helper tests
- Existing narrow target-lowering subset for the selected consumer

Actions:
- Add or extend oracle tests for the selected consumer's Route 1 facts.
- Cover positive producer/constant/materialization answers and fail-closed
  negative cases.
- Keep prepared helpers visible as expected oracle sources.

Completion check:
- Focused tests prove BIR/prepared equivalence for the selected consumer family,
  including no producer, no constant, and non-materializable cases as relevant.

### Step 3: Migrate The Selected Consumer

Goal: Move the selected consumer to `Route1ProducerIndex` or a thin BIR-backed
facade without changing target lowering policy.

Primary targets:
- The selected consumer files from Step 1
- Route 1 query/facade helpers if a narrow adapter is needed

Actions:
- Replace direct prepared lookup reads for the selected semantic facts with
  Route 1 query reads.
- Keep prepared answers only as oracle or compatibility fallback where still
  required.
- Avoid broad scans and avoid duplicating target-owned state in BIR.
- Preserve behavior for unsupported, missing, and non-materializable cases.

Completion check:
- The selected consumer no longer reads the prepared Route 1-adjacent field as
  its primary semantic source, and the narrow proof subset is green.

### Step 4: Contract Only Proven-Private Surface

Goal: Remove or narrow only the prepared cache/API exposure that no longer has
public consumers after the selected migration.

Primary targets:
- `PreparedFunctionLookups` projected fields for the migrated group
- Public includes or context projections made unused by the migration

Actions:
- Re-scan direct consumers before deleting or hiding any field/helper.
- Keep compatibility surfaces public if higher-route, AArch64, x86, or RISC-V
  consumers still require them.
- Limit include/context contraction to behavior-preserving changes.

Completion check:
- Any contraction is backed by consumer evidence and compile proof; no prepared
  helper needed by remaining public consumers is hidden.

### Step 5: Validate And Handoff

Goal: Prove the migration and leave precise lifecycle state for the next packet.

Actions:
- Run the delegated narrow proof command for the selected consumer.
- Run broader backend validation if public headers, aggregate projections, or
  materialization lowering behavior changed.
- Update `todo.md` with proof results, residual consumers, and the next bounded
  Route 1 candidate if one remains.

Completion check:
- Build/proof logs are fresh, `todo.md` reflects the current state, and the
  remaining prepared lookup surfaces are either still public by evidence or
  contracted by a proven consumer migration.
