# Plan: Prepared Call Argument Producer Materializability Contract

Status: Active
Source Idea: ideas/open/122_prepared_call_argument_producer_materializability_contract.md

## Purpose

Move target-neutral call-argument producer materializability and
publication-source routing out of AArch64 calls lowering into a shared prepared
query or fact surface.

## Goal

Make AArch64 calls lowering consume shared prepared facts for call-argument
producer materialization, direct-global select-chain dependencies, and missing
frame-slot argument publication needs where those decisions are target-neutral.

## Core Rule

Shared prepared code owns producer materializability, publication-source
routing, direct-global select-chain dependency facts, and target-neutral
missing frame-slot publication needs. AArch64 calls lowering owns concrete
scalar instruction lowering, emitted-register updates, select-chain assembler
line construction, local aggregate address payload emission, and machine
instruction wrapping.

## Read First

- `ideas/open/122_prepared_call_argument_producer_materializability_contract.md`
- `src/backend/mir/aarch64/codegen/calls.cpp`
- Existing prepared/BIR/prealloc lookup and printer code that exposes call
  argument source or publication facts
- Existing backend tests covering prepared publication, select-chain
  dependencies, call argument producers, local aggregate address arguments, and
  AArch64 scalar call arguments
- Closed idea 116 only if needed to confirm producer/publication authority is
  consumed rather than reopened

## Current Targets

- AArch64 consumer helpers:
  - `lower_scalar_call_argument_producers`
  - `materialize_scalar_call_argument_value`
  - `find_prepared_scalar_call_argument_source_producer`
  - `materialize_direct_global_select_chain_call_argument`
  - `materialize_missing_frame_slot_call_arguments`
- Shared prepared or prealloc surfaces that can own:
  - call-argument source producer materializability
  - publication-source routing
  - direct-global select-chain dependency facts
  - target-neutral missing frame-slot argument publication needs
- Visibility surfaces:
  - prepared printer or route-dump tests
  - focused backend/AArch64 producer and call-argument tests

## Non-Goals

- Do not reopen idea 116 edge-publication producer, current-block publication,
  join-routing, or select-chain contracts.
- Do not add AArch64-only same-block producer discovery, BIR-name matching, or
  direct-global/select-chain named-case shortcuts.
- Do not move AArch64 scalar instruction spelling, register retargeting,
  select-chain assembler spelling, local aggregate address payloads, or
  machine instruction wrapping into shared code.
- Do not split the calls file or perform broad dispatch cleanup before the
  shared query is proven.
- Do not change x86 or RISC-V codegen implementation.

## Working Model

Prepared/shared code should answer whether a call-argument source producer is
materializable for call lowering and which target-neutral publication source or
dependency facts are required. AArch64 calls lowering should consume those
facts, then perform only target-local instruction emission, register updates,
payload construction, and record wrapping.

## Execution Rules

- Keep source idea edits unnecessary unless durable intent changes.
- Characterize the existing AArch64 rediscovery logic before adding a shared
  query.
- Prefer extending an existing prepared lookup, fact, printer, or route-dump
  surface over creating a parallel ad hoc contract.
- Keep AArch64 changes consumer-only once the shared fact exists.
- Preserve local aggregate address emission and scalar machine spelling as
  AArch64-owned behavior.
- Treat expectation downgrades, unsupported-path rewrites, and named-case
  producer matching as route failures.
- For code-changing steps, run a fresh build before claiming readiness.
- Use a focused backend/AArch64 and prepared-query proof subset first.
  Escalate to broader backend proof if shared dispatch, prepared lookup, or
  publication semantics change across consumers.

## Ordered Steps

### Step 1: Characterize Existing Call-Argument Producer Routes

Goal: map the current AArch64-local producer materialization and publication
routing decisions before moving any authority.

Primary targets:

- `src/backend/mir/aarch64/codegen/calls.cpp`
- existing prepared/BIR/prealloc lookup surfaces
- existing tests for scalar call argument producers, select-chain arguments,
  local aggregate address arguments, and missing frame-slot publications

Actions:

- Inspect the current helper flow through
  `lower_scalar_call_argument_producers`,
  `materialize_scalar_call_argument_value`,
  `find_prepared_scalar_call_argument_source_producer`,
  `materialize_direct_global_select_chain_call_argument`, and
  `materialize_missing_frame_slot_call_arguments`.
- Classify which decisions are target-neutral prepared facts versus
  AArch64-specific materialization details.
- Identify existing prepared visibility for publication-source routing,
  direct-global select-chain dependencies, and missing frame-slot argument
  publication needs.
- Identify focused proof candidates and any nearby route gaps.

Completion check:

- `todo.md` records the cluster map, the target-neutral versus AArch64-local
  boundary, existing visibility surfaces, route gaps, and the recommended
  focused proof subset.

### Step 2: Add Shared Producer Materializability Fact Or Query

Goal: expose a target-neutral prepared decision for call-argument source
producer materializability.

Primary targets:

- shared prepared/BIR/prealloc query owners under `src/backend/mir/` or
  `src/backend/prealloc/`
- existing prepared lookup or printer files that expose call argument source
  facts
- focused prepared visibility tests

Actions:

- Add or extend a prepared query or fact that answers whether a call-argument
  source producer is materializable for call lowering.
- Keep the query target-neutral; it must not encode AArch64 scalar instruction
  spelling, scratch-register policy, or assembler-line construction.
- Add printer, dump, or route-test visibility showing the fact for ordinary
  scalar producer routes.
- Keep AArch64 calls lowering unchanged except where a narrow consumer shim is
  needed for compilation or proof.

Completion check:

- Focused proof shows the shared materializability fact, the build passes, and
  `todo.md` records the proof command plus any remaining consumer work.

### Step 3: Add Publication-Source And Select-Chain Dependency Visibility

Goal: make call-argument publication-source routing and direct-global
select-chain dependencies available through the shared prepared surface.

Primary targets:

- shared prepared/BIR/prealloc query or fact owners
- prepared printer or route-dump tests
- existing select-chain call-argument tests

Actions:

- Extend the shared fact or companion query to expose target-neutral
  publication-source routing for call arguments.
- Expose direct-global select-chain dependency facts where AArch64 currently
  rediscovers them.
- Consume existing idea 116 facts instead of duplicating producer,
  current-block publication, join-routing, or select-chain authority.
- Add focused visibility proof for select-chain and neighboring scalar
  producer routes.

Completion check:

- Focused proof shows publication-source routing and direct-global select-chain
  dependency visibility without named-case matching, and `todo.md` records any
  routes that remain intentionally target-local.

### Step 4: Add Missing Frame-Slot Argument Publication Visibility

Goal: expose target-neutral missing frame-slot argument publication needs
through prepared facts when they are not AArch64-local emission decisions.

Primary targets:

- shared prepared/BIR/prealloc query or fact owners
- `materialize_missing_frame_slot_call_arguments` as the AArch64 consumer
- tests for frame-slot argument publication and local aggregate address
  argument routes

Actions:

- Identify which missing frame-slot publication decisions are target-neutral
  and which are tied to AArch64 emission or local aggregate address payloads.
- Extend the shared prepared surface for target-neutral missing frame-slot
  needs.
- Keep local aggregate address payload construction and machine record
  wrapping in AArch64 calls lowering.
- Add focused proof covering missing frame-slot publication and neighboring
  aggregate-address routes.

Completion check:

- Focused proof shows missing frame-slot publication needs are prepared facts
  or records a reviewed keep-local decision, the build passes, and `todo.md`
  records the exact proof run.

### Step 5: Convert AArch64 Calls To Consume The Shared Surface

Goal: remove AArch64 rediscovery of target-neutral producer routes while
leaving target-specific emission local.

Primary targets:

- `src/backend/mir/aarch64/codegen/calls.cpp`
- shared query/fact consumers
- focused backend/AArch64 producer and call-argument tests

Actions:

- Update AArch64 calls lowering to consume the shared materializability,
  publication-source, select-chain dependency, and missing frame-slot facts.
- Remove or narrow old AArch64-local rediscovery logic only when the shared
  surface fully owns that decision.
- Preserve scalar instruction lowering, emitted-register updates,
  select-chain assembler line construction, local aggregate address payload
  emission, and machine instruction wrapping as AArch64-local behavior.
- Avoid broad calls cleanup or file splitting outside this contract.

Completion check:

- AArch64 calls lowering consumes the shared facts, focused backend/AArch64
  proof covers ordinary scalar producers, direct-global select-chain call
  arguments, local aggregate address call arguments, and missing frame-slot
  publication, and `todo.md` records the proof.

### Step 6: Acceptance Review Package

Goal: decide whether idea 122 is complete under its source criteria.

Actions:

- Compare the final diff against the source idea and reviewer reject signals.
- Confirm AArch64 calls no longer rediscover target-neutral producer
  materializability or publication-source routes covered by the shared fact.
- Confirm direct-global select-chain dependency and missing frame-slot
  publication needs are visible as prepared facts or have reviewed keep-local
  rationale.
- Confirm no AArch64 same-block producer scan, BIR-name matching,
  expectation downgrade, or named-case select-chain shortcut was accepted as
  progress.
- Run or recommend the supervisor-selected close proof.

Completion check:

- `todo.md` records whether the source idea appears closure-ready, what proof
  was run, and any residual follow-up needed before lifecycle closure.
