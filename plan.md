# BIR Annotation Lookup Index Schema Plan

Status: Active
Source Idea: ideas/open/166_bir_annotation_lookup_index_schema.md

## Purpose

Prototype function-local lookup indexes over existing Route 1 through Route 7
BIR annotation records while keeping value, instruction, terminator, block,
and edge annotation payloads as the semantic source of truth.

## Goal

Consumers retain cheap prepared-query-style lookup by stable BIR ids without
adding a durable `PreparedFunctionLookups` clone or duplicating semantic
payloads in a function-level lowering-plan container.

## Core Rule

Indexes are references and accelerators only. Do not store full semantic
payloads that belong on typed annotations, and do not introduce ABI, layout,
register allocation, move scheduling, call plan, memory-addressing, frame,
dynamic-stack, helper, final-emission, storage-home, or target lowering policy
metadata into function-level BIR indexes.

## Read First

- `ideas/open/166_bir_annotation_lookup_index_schema.md`
- `docs/bir_prealloc_fusion/phase_b_annotation_schema_candidates.md`
- `src/backend/bir/bir.hpp`
- `src/backend/bir/bir.cpp`
- `src/backend/mir/query.hpp`
- `src/backend/mir/query.cpp`
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`

## Current Targets

- Function-local indexes that reference typed Route 1 through Route 7
  annotation records.
- Route-specific lookup keys for function, block label, instruction index,
  value/name, call-site ordinal, CFG edge key, before-instruction index, and
  relationship kind.
- Rebuild or validation checks that prove index entries cannot diverge from the
  annotation payloads they reference.
- Private aggregate facades that preserve lookup ergonomics while consumers
  migrate one relationship at a time.

## Non-Goals

- Do not add a durable BIR-owned `PreparedFunctionLookups` aggregate.
- Do not add function-level ABI, layout, register allocation, move scheduling,
  call plan, memory-addressing, frame, dynamic-stack, helper, final-emission,
  storage-home, or target policy data.
- Do not duplicate semantic payload fields in indexes when typed annotations
  already own those payloads.
- Do not invent a new Phase A semantic route; index only accepted Route 1
  through Route 7 relationships.
- Do not claim helper renames, facade movement, or expectation rewrites as
  progress without typed annotation references and divergence checks.

## Working Model

- Typed Route 1 through Route 7 annotation records remain the source of truth.
- Function-local indexes own only stable lookup keys and references into typed
  annotation records.
- Rebuild/validation helpers prove the index can be regenerated from
  annotations and fails closed when records are missing or divergent.
- Aggregate facades stay private and route consumers through typed indexes
  without becoming a lowering-plan container.

## Execution Rules

- Keep each code-changing step small enough for a narrow backend proof.
- Prefer extending or unifying existing Route 1 through Route 7 index helpers
  over creating a parallel aggregate structure.
- Add divergence and fail-closed coverage alongside positive lookup coverage.
- Preserve current prepared-query lookup shapes only where performance or
  migration ergonomics require them.
- Run `git diff --check` for every slice.
- For code-changing slices, run `cmake --build --preset default` plus the
  delegated narrow CTest subset and write the grouped output to
  `test_after.log`.
- Escalate to a broader backend subset before closure because this plan spans
  all Phase B route indexes and shared query helpers.

## Steps

### Step 1: Inventory Route Index Surfaces

Goal: identify existing Route 1 through Route 7 index helpers, lookup keys,
payload ownership boundaries, and proof tests before adding shared schema.

Primary targets:

- `src/backend/bir/bir.hpp`
- `src/backend/bir/bir.cpp`
- `src/backend/mir/query.hpp`
- `src/backend/mir/query.cpp`
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`

Actions:

- Inspect existing route-specific index structs, query structs, build helpers,
  lookup helpers, and any private aggregate facades.
- Record the route-by-route lookup keys, typed record references, payload
  ownership boundary, divergence checks, positive cases, negative cases, and
  the narrow proof command in `todo.md`.
- Identify the first low-risk index contract to normalize without changing
  consumer semantics.

Completion check:

- `todo.md` names concrete Route 1 through Route 7 index surfaces, the
  record-reference contract, divergence/fail-closed cases, and delegated proof
  command for Step 2.

### Step 2: Define Shared Index Reference Contracts

Goal: introduce common index-reference contracts over typed route records
without moving semantic payloads out of annotations.

Primary targets:

- `src/backend/bir/bir.hpp`
- `src/backend/bir/bir.cpp`
- selected narrow BIR/backend tests from Step 1

Actions:

- Add or normalize function-local index reference structures that identify
  route, relationship kind, stable BIR key, and referenced typed annotation
  record.
- Keep semantic fields in the existing typed Route 1 through Route 7 records.
- Add validation helpers that reject missing records, stale references,
  duplicate key/reference ambiguity, and payload/index divergence.
- Add tests proving index references can be rebuilt from annotation records and
  do not expose lowering-policy payloads.

Completion check:

- Narrow backend tests prove the shared reference contract points at typed
  annotation payloads, rebuilds or validates from those payloads, and fails
  closed for divergent entries.

### Step 3: Normalize Route-Specific Lookup Helpers

Goal: adapt selected route-specific lookup helpers to use the shared reference
contract while preserving their public answer shapes.

Primary targets:

- `src/backend/bir/bir.hpp`
- `src/backend/bir/bir.cpp`
- selected narrow BIR/backend tests from Step 1

Actions:

- Normalize route lookup helpers only where existing helpers already depend on
  typed annotation records.
- Preserve route-specific query keys such as block label, value/name,
  instruction index, edge key, call-site ordinal, before-instruction index, and
  relationship kind.
- Keep aggregate facades private and avoid introducing a route-independent
  semantic payload container.
- Prove positive lookup, missing route record, duplicate key, stale reference,
  and wrong relationship kind behavior.

Completion check:

- Tests demonstrate that normalized helpers retain cheap lookup identity,
  reference typed annotation records, and fail closed without falling back to
  prepared-only scans.

### Step 4: Migrate A Low-Risk Consumer Facade

Goal: switch the narrowest private aggregate or shared lookup facade to the
normalized index-reference path while keeping route-specific typed records as
source of truth.

Primary targets:

- `src/backend/bir/bir.hpp`
- `src/backend/bir/bir.cpp`
- `src/backend/mir/query.cpp`
- backend tests identified in Step 1

Actions:

- Replace one low-risk helper/facade path that currently rebuilds or scans
  route records manually with the normalized index-reference lookup.
- Keep public answer shapes and prepared-oracle comparisons unchanged.
- Avoid broad MIR, target/codegen, ABI/layout, prealloc, or final-emission
  consumer switches unless already proven by the selected helper.

Completion check:

- Narrow tests prove the migrated helper/facade returns the same answers for
  positive, missing, divergent, duplicate, and no-match cases selected in Step
  1.

### Step 5: Broaden Validation And Prepare Closure

Goal: prove the function-local annotation lookup index schema is complete
enough for source-idea closure.

Primary targets:

- `todo.md`
- `test_before.log`
- `test_after.log`

Actions:

- Run or request the broader backend subset chosen by the supervisor because
  this route spans shared BIR indexes and query helpers.
- Confirm no durable `PreparedFunctionLookups` clone, semantic payload
  duplication, target-policy import, broad consumer drift, or helper-only
  reshuffle remains.
- Update `todo.md` with closure readiness, proof commands, and residual risks.

Completion check:

- Regression evidence is fresh for the accepted scope, and the source idea's
  acceptance criteria are satisfied without testcase-shaped shortcuts.
