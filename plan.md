# Plan: Prepared Call Result Late Publication Contract

Status: Active
Source Idea: ideas/open/123_prepared_call_result_late_publication_contract.md

## Purpose

Move target-neutral call-result late-publication and retargeting decisions out
of AArch64 calls lowering into a shared prepared query or fact surface.

## Goal

Make AArch64 calls lowering consume shared prepared facts for call-result
source-register publication, source-in-destination aliasing, FPR/VREG
store-value retargeting, and current-block publication consumption where those
decisions are target-neutral.

## Core Rule

Shared prepared code owns the target-neutral decision describing when a call
result source can publish, alias, or retarget to an already-emitted scalar.
AArch64 calls lowering owns concrete FP/f128/GP materialization lines,
scratch-register choice, q/vector move spelling, memory-store record emission,
and call-boundary machine records.

## Read First

- `ideas/open/123_prepared_call_result_late_publication_contract.md`
- `src/backend/mir/aarch64/codegen/calls.cpp`
- Existing prepared/prealloc lookup and printer code that exposes call result,
  current-block publication, source-producer, and value-home facts
- Existing backend tests covering call result publication, current-block
  publication, FPR/VREG result stores, and AArch64 call-boundary moves
- Closed idea 117 only if needed to confirm comparison/current-block
  publication authority is consumed rather than reopened
- Closed idea 116 only if needed to confirm producer/publication authority is
  consumed rather than reopened

## Current Targets

- AArch64 consumer helpers:
  - `record_call_result_source_register`
  - `retarget_fpr_call_result_store_value_to_emitted_scalar`
  - `materialize_call_boundary_source_to_destination`
  - `retarget_call_boundary_source_to_emitted_scalar`
  - `record_call_boundary_destination`
  - `record_call_boundary_source_in_destination`
- Shared prepared or prealloc surfaces that can own:
  - call-result late-publication availability
  - call-result source-register publication routing
  - source-in-destination alias decisions
  - FPR/VREG store-value retargeting decisions
  - current-block publication consumption from idea 117 facts
- Visibility surfaces:
  - prepared printer or route-dump tests
  - focused backend/AArch64 call-result and call-boundary tests

## Non-Goals

- Do not reopen idea 117 fused-compare, materialized-compare, or
  current-block publication authority.
- Do not reopen idea 116 prepared producer or same-block producer authority.
- Do not recompute destination homes or stack storage covered by idea 93.
- Do not move AArch64 FP/f128/GP materialization lines, scratch-register
  policy, q/vector move spelling, or call-boundary machine record emission into
  shared code.
- Do not perform broad calls lowering cleanup, comparison cleanup, or dispatch
  cleanup not needed for this prepared query.

## Working Model

Prepared/shared code should answer whether a call result can be published late,
aliased through source-in-destination state, or retargeted to an already
emitted scalar. AArch64 calls lowering should consume those facts and then
perform only target-local instruction emission, scalar-state updates, payload
construction, and machine-record wrapping.

## Execution Rules

- Keep source idea edits unnecessary unless durable intent changes.
- Characterize the existing AArch64 result-publication cluster before adding a
  shared query.
- Prefer extending an existing prepared lookup, fact, printer, or route-dump
  surface over creating a parallel ad hoc contract.
- Keep AArch64 changes consumer-only once the shared fact exists.
- Preserve FP/f128/GP materialization, scratch selection, q/vector spelling,
  memory-store records, and call-boundary machine records as AArch64-owned
  behavior.
- Treat expectation downgrades, unsupported-path rewrites, and named-case
  result-retargeting shortcuts as route failures.
- For code-changing steps, run a fresh build before claiming readiness.
- Use a focused backend/AArch64 and prepared-query proof subset first.
  Escalate to broader backend proof if shared publication semantics change
  across comparison or dispatch consumers.

## Ordered Steps

### Step 1: Characterize Existing Call-Result Late-Publication Routes

Goal: map the current AArch64-local call-result publication, aliasing, and
retargeting decisions before moving any authority.

Primary targets:

- `src/backend/mir/aarch64/codegen/calls.cpp`
- existing prepared/prealloc lookup and printer surfaces
- existing tests for call result source recording, source-in-destination
  aliasing, FPR/VREG result stores, and current-block publication

Actions:

- Inspect the current helper flow through
  `record_call_result_source_register`,
  `retarget_fpr_call_result_store_value_to_emitted_scalar`,
  `materialize_call_boundary_source_to_destination`,
  `retarget_call_boundary_source_to_emitted_scalar`,
  `record_call_boundary_destination`, and
  `record_call_boundary_source_in_destination`.
- Classify which decisions are target-neutral prepared facts versus
  AArch64-specific materialization details.
- Identify existing prepared visibility for call results, current-block
  publication, source-in-destination aliases, and result-store retargeting.
- Identify focused proof candidates and nearby route gaps.

Completion check:

- `todo.md` records the cluster map, the target-neutral versus AArch64-local
  boundary, existing visibility surfaces, route gaps, and the recommended
  focused proof subset.

### Step 2: Add Shared Call-Result Late-Publication Fact Or Query

Goal: expose a target-neutral prepared decision for call-result late
publication and retargeting.

Primary targets:

- shared prepared/BIR/prealloc query owners under `src/backend/mir/` or
  `src/backend/prealloc/`
- existing prepared lookup or printer files that expose call result and
  current-block publication facts
- focused prepared visibility tests

Actions:

- Add or extend a prepared query or fact that answers whether a call result can
  publish late or retarget to an already-emitted scalar.
- Keep the query target-neutral; it must not encode AArch64 materialization
  lines, scratch-register policy, q/vector spelling, or memory-store record
  construction.
- Consume idea 117 current-block publication facts instead of duplicating
  comparison publication routing.
- Add printer, dump, or route-test visibility showing the late-publication
  decision for ordinary call-result routes.
- Keep AArch64 calls lowering unchanged except where a narrow consumer shim is
  needed for compilation or proof.

Completion check:

- Focused proof shows the shared late-publication fact, the build passes, and
  `todo.md` records the proof command plus any remaining consumer work.

### Step 3: Add Source-In-Destination And Source-Register Visibility

Goal: make source-register recording and source-in-destination alias decisions
available through the shared prepared surface.

Primary targets:

- shared prepared/prealloc query or fact owners
- `record_call_result_source_register`
- `record_call_boundary_source_in_destination`
- prepared printer or route-dump tests
- existing call-boundary and call-result tests

Actions:

- Extend the shared fact or companion query to expose target-neutral
  source-register publication and source-in-destination alias decisions.
- Keep scalar-state recording and concrete call-boundary machine records in
  AArch64 calls lowering.
- Add focused visibility proof for source-register and source-in-destination
  routes.

Completion check:

- Focused proof shows source-register and source-in-destination visibility
  without reopening producer/current-block authority, and `todo.md` records any
  routes that remain intentionally target-local.

### Step 4: Add FPR/VREG Store-Value Retargeting Visibility

Goal: expose target-neutral FPR/VREG result-store retargeting decisions through
prepared facts when they are not AArch64-local emission decisions.

Primary targets:

- shared prepared/prealloc query or fact owners
- `retarget_fpr_call_result_store_value_to_emitted_scalar`
- tests for FPR/VREG result stores and neighboring call-result routes

Actions:

- Identify which FPR/VREG store-value retargeting decisions are
  target-neutral and which are tied to AArch64 register views or store record
  payloads.
- Extend the shared prepared surface for target-neutral result-store
  retargeting.
- Keep FP/f128/q-vector spelling, memory-store record mutation, and scalar
  register views in AArch64 calls lowering.
- Add focused proof covering FPR/VREG store retargeting and neighboring
  call-result publication routes.

Completion check:

- Focused proof shows FPR/VREG store-value retargeting needs are prepared facts
  or records a reviewed keep-local decision, the build passes, and `todo.md`
  records the exact proof run.

### Step 5: Convert AArch64 Calls To Consume The Shared Surface

Goal: remove AArch64 rediscovery of target-neutral result-publication and
retargeting routes while leaving target-specific emission local.

Primary targets:

- `src/backend/mir/aarch64/codegen/calls.cpp`
- shared query/fact consumers
- focused backend/AArch64 call-result and call-boundary tests

Actions:

- Update AArch64 calls lowering to consume the shared late-publication,
  source-register, source-in-destination, FPR/VREG retargeting, and
  current-block publication facts.
- Remove or narrow old AArch64-local rediscovery logic only when the shared
  surface fully owns that decision.
- Preserve FP/f128/GP materialization, emitted-register updates,
  scratch-register policy, q/vector spelling, memory-store record emission,
  and call-boundary machine records as AArch64-local behavior.
- Avoid broad calls cleanup or file splitting outside this contract.

Completion check:

- AArch64 calls lowering consumes the shared facts, focused backend/AArch64
  proof covers source-register recording, source-in-destination aliasing,
  FPR/VREG store-value retargeting, and current-block publication consumption,
  and `todo.md` records the proof.

### Step 6: Acceptance Review Package

Goal: decide whether idea 123 is complete under its source criteria.

Actions:

- Compare the final diff against the source idea and reviewer reject signals.
- Confirm AArch64 calls no longer rebuilds current-block publication,
  comparison publication, or same-block producer facts for covered
  late-publication routes.
- Confirm source-register recording, source-in-destination aliasing, and
  FPR/VREG store-value retargeting are visible as prepared facts or have
  reviewed keep-local rationale.
- Confirm no expectation downgrade, unsupported-path rewrite, named-case result
  shortcut, or target-specific materialization move into shared code was
  accepted as progress.
- Run or recommend the supervisor-selected close proof.

Completion check:

- Reviewer-quality notes and proof are recorded in `todo.md`, and the
  supervisor can decide whether to close, split, or continue the source idea.
