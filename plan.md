# BIR Producer/Source Identity Foundation Runbook

Status: Active
Source Idea: ideas/open/152_bir_producer_source_identity_foundation.md

## Purpose

Create the target-neutral BIR producer/source identity foundation required
before later BIR normalization routes can leave prepared/prealloc ownership.

## Goal

Add BIR-owned same-block producer and constant identity queries that can be
proven equivalent to the existing prepared query surface before any consumer is
switched.

## Core Rule

Keep BIR identity semantic and target-neutral. Do not import register homes,
storage state, publication-owned enum authority, emitted-register availability,
spill/reload behavior, or final instruction order into the BIR schema.

## Read First

- `ideas/open/152_bir_producer_source_identity_foundation.md`
- `docs/bir_prealloc_fusion/phase_a_normalization_candidates.md`
- existing prepared-query owners for:
  - `find_prepared_same_block_scalar_producer`
  - `evaluate_prepared_same_block_integer_constant`

## Current Targets

- Same-block scalar producer identity.
- Same-block immediate/integer constant identity.
- A BIR-owned producer-kind vocabulary.
- Query-cache or per-function relationship keyed by block label,
  before-instruction index, value/name/type, producer node, immediate constants,
  and materialization availability.
- At most one narrow low-risk consumer switch after equivalence is proven.

## Non-Goals

- Do not redesign register allocation, spill/reload, publication plans, storage
  encoding, operand views, or final MIR emission ordering.
- Do not replace broad MIR emission routes before BIR/prepared equivalence
  exists.
- Do not downgrade tests, weaken supported-path expectations, or claim progress
  through expectation rewrites.

## Working Model

- The prepared query surface remains the oracle while BIR-owned queries are
  introduced.
- BIR queries must cover the same semantic subset as prepared same-block scalar
  and constant queries on representative scalar, cast, binary, load, select,
  and constant cases.
- Consumer switching is optional and must be narrow, read-only, and backed by
  query-equivalence proof.

## Execution Rules

- Prefer small implementation packets with matching build/test proof.
- Keep source-idea edits unnecessary unless the durable intent changes.
- Use `todo.md` for packet progress and proof notes.
- Preserve `test_before.log` and `test_after.log` as the canonical matched
  regression logs when a packet changes code.
- Treat testcase-shaped shortcuts, named-case-only fixes, and expectation
  downgrades as route failures.

## Ordered Steps

### Step 1: Locate Prepared Query Surface And Evidence Set

Goal: establish the current query owners, callers, and representative proof
cases before adding BIR-owned state.

Primary target: prepared same-block producer and integer-constant query code.

Actions:
- Locate definitions and direct callers of
  `find_prepared_same_block_scalar_producer` and
  `evaluate_prepared_same_block_integer_constant`.
- Identify the data each prepared query consumes and returns.
- Identify representative scalar, cast, binary, load, select, and constant
  cases that can prove BIR/prepared equivalence.
- Record the narrow backend/codegen subset the executor should use for the
  first implementation packet.

Completion check:
- `todo.md` names the inspected files/functions, candidate proof cases, and the
  proposed first code-changing packet.

### Step 2: Define Target-Neutral BIR Producer Vocabulary

Goal: add a BIR-owned producer-kind vocabulary without depending on prepared,
publication, register, storage, or emission enum authority.

Primary target: BIR identity/query types near the existing BIR query or
function-analysis surface.

Actions:
- Define the minimal producer-kind vocabulary required by the accepted semantic
  subset.
- Represent value/name/type, block label, before-instruction index, producer
  node, immediate constant, and materialization availability facts.
- Keep the model independent of register names, homes, operand views, spill
  slots, and final instruction order.
- Add focused unit or dump coverage when the local test structure supports it.

Completion check:
- The new BIR vocabulary compiles, is reachable from the intended query owner,
  and does not import rejected prepared/prealloc/publication authority.

### Step 3: Add BIR-Owned Same-Block Producer Queries

Goal: expose BIR-owned queries that can answer the same semantic subset as the
prepared same-block and integer-constant queries.

Primary target: per-function analysis or query-cache implementation.

Actions:
- Build or populate the BIR relationship over block label, before-instruction
  index, value/name/type, producer node, immediate constants, and
  materialization availability.
- Add query APIs for same-block scalar producer and integer constant identity.
- Keep the old prepared queries available as the comparison oracle.
- Avoid consumer rewrites in this step except for test-only comparison hooks.

Completion check:
- BIR queries compile and can be compared against prepared query results without
  changing broad backend behavior.

### Step 4: Prove Prepared/BIR Query Equivalence

Goal: demonstrate that the new BIR queries match the prepared oracle for the
  supported producer kinds.

Primary target: targeted backend/codegen tests, assertions, or dumps.

Actions:
- Add or update targeted proof so scalar, cast, binary, load, select, and
  constant cases compare BIR results against prepared results.
- Capture matched `test_before.log` and `test_after.log` for the smallest
  backend/codegen subset that exercises same-block scalar materialization.
- Investigate any mismatch as a semantic gap instead of special-casing the
  named failing case.

Completion check:
- Matched proof logs show no regression, and equivalence evidence covers each
  supported producer kind named by the source idea.

### Step 5: Switch At Most One Narrow Consumer

Goal: move one low-risk identity reader to the BIR query only after equivalence
proof exists.

Primary target: one narrow consumer that only reads already-proven identity
facts.

Actions:
- Select a consumer with minimal blast radius and no register/storage emission
  ownership.
- Switch only identity reads already proven equivalent.
- Keep prepared queries available for comparison and fallback during this idea.
- Run the delegated narrow subset and escalate to broader backend validation if
  the selected consumer affects shared codegen behavior.

Completion check:
- One narrow consumer uses the BIR query for proven identity facts, matched
  proof remains green, and no broad MIR emission rewrite is included.

### Step 6: Acceptance Review And Close Payload

Goal: make closure evidence explicit for supervisor and reviewer handoff.

Actions:
- Summarize the BIR vocabulary, query API, equivalence proof cases, and any
  consumer switch in `todo.md`.
- Confirm the prepared query oracle remains available.
- Confirm no rejected target-specific state entered the BIR relationship.
- Confirm matched regression logs exist for the final code-changing scope.

Completion check:
- `todo.md` contains the close-ready evidence summary and proof commands needed
  for lifecycle closure review.
