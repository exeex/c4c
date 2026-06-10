# BIR Call-Boundary Source Facts Runbook

Status: Active
Source Idea: ideas/open/157_bir_call_boundary_source_facts.md

## Purpose

Move target-neutral call-boundary source, dependency, and producer facts toward
BIR ownership while preserving ABI placement and target lowering in
prealloc/codegen.

## Goal

Add BIR-owned call-use source relationship queries for call arguments and
results that can match the semantic parts of prepared call plans before any
consumer switch.

## Core Rule

Only migrate semantic call-boundary facts: source value/base value/name,
target-neutral pointer delta, materializable same-block producer,
direct-global select-chain dependency, memory/access source link, and
publication-source route identity. Do not import ABI registers, stack areas,
aggregate lanes, scratch requirements, preservation/clobber state,
destination homes, helper/carrier protocols, or aggregate transport layout
into BIR.

## Read First

- `ideas/open/157_bir_call_boundary_source_facts.md`
- `docs/bir_prealloc_fusion/phase_a_normalization_candidates.md`
- `src/backend/prealloc/calls.hpp`
- `src/backend/prealloc/call_plans.cpp`
- `src/backend/prealloc/prepared_lookups.cpp`
- `src/backend/mir/aarch64/codegen/calls.cpp`
- `tests/backend/bir/backend_prealloc_call_boundary_classification_test.cpp`
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`
- nearby backend call-boundary case tests under `tests/backend/case/`

## Current Targets

- Prepared oracle surfaces:
  - `PreparedCallArgumentSourceSelection`
  - `PreparedCallArgumentSourceProducerMaterialization`
  - `PreparedCallArgumentDirectGlobalSelectChainDependency`
  - `PreparedCallArgumentPublicationSourceRouting`
  - call-result publication/source facts where they expose semantic source
    identity without ABI placement
- Prepared oracle helpers:
  - `find_prepared_call_argument_source_producer_materialization`
  - `find_prepared_call_argument_direct_global_select_chain_dependency`
  - `find_prepared_call_argument_publication_source_routing`
  - call-boundary source lookup helpers used by AArch64 call materialization
- BIR relationship surface attached to `bir::CallInst` arguments and results.
- Call materialization/routing consumers only after equivalence is proven for
  the specific argument or result class being switched.

## Non-Goals

- Do not change calling-convention placement, destination register names,
  source/result register names, outgoing stack sizing, stack offsets/sizes,
  variadic FPR count, preservation/clobber policy, byval aggregate transport,
  aggregate lane layout, scratch requirements, helper/carrier protocols, or
  final call lowering.
- Do not copy `PreparedCallArgumentPlan`, `PreparedCallPlan`, aggregate
  transport structs, or full publication/move records into BIR.
- Do not rewrite comparison, memory, edge-publication, stack-layout, regalloc,
  or target instruction selection behavior as part of this idea.
- Do not weaken tests, downgrade supported paths, or add named-testcase
  shortcuts to claim progress.

## Working Model

- Prepared call plans remain the oracle until BIR answers match one call-use
  fact family at a time.
- BIR may own the relationship: "this call argument/result use is sourced from
  this BIR value or producer, with these target-neutral dependency links."
- Prealloc/codegen continues to own how that source becomes ABI argument moves,
  result moves, stack copies, aggregate transport, and final emitted code.

## Execution Rules

- Work in narrow, observable steps. Add or expose one call-use fact family
  before switching consumers to it.
- For each consumer migration, prove old prepared answers and new BIR answers
  are equivalent first.
- Keep comparison against the prepared oracle until the current step has nearby
  positive, negative, scalar, aggregate, and indirect/source-dependent coverage
  appropriate to that fact family.
- If implementation requires touching ABI placement, aggregate transport,
  scratch, preservation/clobber, or move-ordering code, stop and escalate to
  the supervisor for route review and broader validation.
- Use matching before/after backend call/codegen coverage when the supervisor
  delegates proof. Escalate to broader backend validation if ABI, aggregate,
  or target lowering files change.

## Ordered Steps

### Step 1: Inspect the prepared call-boundary oracle

Goal: define the semantic call-use field boundary before adding BIR-facing
state.

Primary target: prepared call-source helpers and existing call-boundary tests.

Actions:

- Inspect `PreparedCallArgumentSourceSelection`,
  `PreparedCallArgumentSourceProducerMaterialization`,
  `PreparedCallArgumentDirectGlobalSelectChainDependency`, and
  `PreparedCallArgumentPublicationSourceRouting`.
- Inspect call-result publication/source facts that expose source identity
  without requiring ABI placement.
- Classify fields into BIR-owned semantic identity versus prealloc/ABI/codegen
  placement or routing policy.
- Identify existing coverage for scalar arguments, stack/frame-slot sources,
  direct-global select-chain dependency, publication-source routing,
  aggregate/byval source cases, result publication, and unavailable-source
  paths.
- Record any missing narrow coverage in `todo.md` before implementing code.

Completion check:

- The executor can name the exact call argument/result source facts to model
  in BIR and the exact prepared fields that must remain outside BIR.

### Step 2: Add the BIR call-use source relationship surface

Goal: expose a BIR-owned query shape attached to call arguments and results.

Primary target: BIR relationship or analysis-cache code adjacent to existing
BIR function/block/value metadata.

Actions:

- Add a target-neutral call-use source record with only the allowed semantic
  fields.
- Include source value/base value/name and target-neutral pointer delta where
  the source is semantic pointer arithmetic.
- Include materializable same-block producer identity where available.
- Include direct-global select-chain dependency and publication-source route
  identity as semantic links, not as target materialization policy.
- Include memory/access source links only as source identity, not as stack
  layout, addressing-form, or ABI placement.
- Add construction or extraction logic without switching production consumers.

Completion check:

- The project builds, and tests can query the BIR relationship without
  changing call argument/result lowering behavior.

### Step 3: Prove prepared-oracle equivalence for argument source facts

Goal: compare BIR call-argument source answers against the prepared oracle
before any argument materialization consumer relies on BIR.

Primary target: backend BIR prepared-lookup tests and narrow call-boundary
coverage.

Actions:

- Add equivalence checks for scalar call-argument source selection.
- Add equivalence checks for same-block source-producer materialization.
- Add equivalence checks for direct-global select-chain dependency.
- Add equivalence checks for publication-source routing and unavailable-source
  paths.
- Include nearby stack/frame-slot, aggregate/byval, and negative cases when
  those prepared facts expose semantic source identity.
- Keep prepared call plans as the source of truth during this step.

Completion check:

- Narrow backend BIR coverage proves BIR argument answers match prepared
  semantic call-source answers without changing ABI placement or call lowering.

### Step 4: Prove prepared-oracle equivalence for result source facts

Goal: cover call-result source and publication identity without pulling result
ABI placement into BIR.

Primary target: call-result publication/source helpers and their backend tests.

Actions:

- Add BIR representation or query support for result source identity where the
  prepared oracle exposes target-neutral source facts.
- Compare against prepared result publication/source answers.
- Exclude result register names, destination homes, stack slots, preservation
  routes, and result move records.
- Add positive and negative coverage for result publication/source paths.

Completion check:

- BIR result-source answers match prepared semantic source facts, and no result
  ABI binding fields are copied into BIR.

### Step 5: Switch one call-source consumer at a time

Goal: migrate reads only after BIR/prepared equivalence is proven for the
specific fact family.

Primary target: call materialization and routing reads selected by the
supervisor packet.

Actions:

- Switch a single source-producer materialization, direct-global dependency, or
  publication-source routing consumer to the BIR query.
- Keep ABI move planning, aggregate transport, scratch selection, and final
  lowering downstream in prealloc/codegen.
- Retain fallback or comparison checks where needed until equivalent coverage
  is broad enough to remove them.
- Repeat only after the previous consumer has build and narrow proof.

Completion check:

- The switched consumer reads semantic call-boundary source identity from BIR
  while generated call behavior remains unchanged against prepared-oracle
  tests.

### Step 6: Run the acceptance proof

Goal: demonstrate that the idea's semantic migration is complete enough for
supervisor review.

Primary target: backend call-boundary, prepared lookup, and AArch64 call/codegen
coverage chosen by the supervisor.

Actions:

- Run matching before/after backend coverage for call-boundary source facts.
- Include same-block producer, direct-global select-chain, memory-source,
  publication-source, result-source, and negative call-argument cases.
- Escalate to broader backend validation if ABI placement, aggregate transport,
  scratch, preservation/clobber, or target lowering files changed.

Completion check:

- Acceptance proof is green, BIR call queries match prepared semantic answers,
  and call lowering retains prealloc/codegen ownership of ABI placement and
  target-specific emission.

## Reviewer Reject Signals

- Imports `PreparedCallArgumentPlan`, `PreparedCallPlan`, aggregate transport
  shapes, ABI placement, scratch policy, variadic state, preservation/clobber
  state, helper/carrier protocols, or prepared move records into BIR.
- Changes ABI placement, aggregate call transport, or final call lowering while
  claiming semantic source-fact migration.
- Switches a consumer before proving BIR/prepared equivalence for that specific
  argument or result fact family.
- Validates only one named call shape without nearby scalar, stack/frame-slot,
  aggregate/byval, result, direct-global, publication-source, and negative
  coverage appropriate to the touched path.
- Rewrites expectations, downgrades supported tests, or adds testcase-shaped
  matching instead of modeling the semantic call-use relationship generally.
