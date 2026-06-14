# 262 Phase F3 x86 Compare-Join LoadLocal selected-arm support

## Idea Type

x86 backend production support for one materialized compare-join source shape.

## Goal

Teach the x86 prepared compare-join return path to classify and render a
selected arm rooted in a same-predecessor-block `LoadLocal` through the existing
Route 3 statement-memory facade.

## Why This Exists

Idea 261 found a supported publication route for the idea 258 proof surface:
the joined-branch compare-join `EdgeStoreSlot` path can naturally produce a
selected `LoadLocal` `source_memory_access` publication. The route still fails
before the agreement facade because the materialized compare-join return
contract only accepts selected arms rooted in immediates, params,
same-module globals, or pointer-backed globals.

This idea exists to add the missing production compare-join `LoadLocal`
selected-arm capability without turning idea 261 into fixture-only or
synthetic publication work.

## In Scope

- Extend the prepared materialized compare-join return classification/render
  contract for one selected-arm shape: same-predecessor-block `LoadLocal` with
  prepared local-slot addressing.
- Render that selected arm through
  `render_agreed_route3_load_local_statement_memory_operand(...)` or a narrow
  helper that preserves its Route 3/prepared agreement authority.
- Preserve the existing compare-join paths for immediate, param, same-module
  global, pointer-backed global, and trailing immediate-binary shapes.
- Add focused x86 handoff tests proving the positive selected-`LoadLocal` arm
  and the route rejection behavior that prevents fallback past authoritative
  prepared metadata.
- Record when idea 261 can resume its fixture/proof-surface work.

## Out Of Scope

- Generic compare-join lowering or broad selected-value expression support.
- Rewriting x86 addressing, register materialization, frame layout, or emitted
  output policy outside the selected `LoadLocal` arm.
- Making prepared `memory_accesses` the semantic authority for Route 3
  selected-memory facts.
- RISC-V behavior changes.
- Synthetic `PreparedEdgePublication` or `join_transfers` injection that the
  production route would reject.
- Weakening existing compare-join status, route-debug, fallback, or output
  contracts.

## Acceptance Criteria

- A joined-branch compare-join route with a same-predecessor-block selected
  `LoadLocal` arm reaches the Route 3 statement-memory agreement facade.
- Positive x86 tests prove the selected `LoadLocal` arm renders through the
  supported compare-join route without synthetic publication injection.
- Negative tests prove missing, incomplete, drifted, or unsupported
  authoritative prepared compare-join metadata fails closed instead of falling
  back past the handoff.
- Existing prepared lookup/printer tests and x86 route-debug/handoff-boundary
  tests continue to pass.
- `todo.md` records that idea 261 can resume, or records the remaining blocker
  if this selected-arm support is insufficient.

## Reviewer Reject Signals

- Reject named-fixture matching or output-spelling shortcuts that do not add a
  real compare-join selected-`LoadLocal` classification/render path.
- Reject hand-built `PreparedEdgePublication` or `join_transfers` rows that
  bypass the production prepared compare-join route.
- Reject tests that claim progress by proving only legacy no-publication
  fallback instead of positive Route 3/prepared agreement.
- Reject broad compare-join rewrites, generic expression lowering, or target
  policy changes beyond the selected `LoadLocal` arm.
- Reject expectation downgrades, helper/oracle renames, route-debug weakening,
  fallback-name rewrites, or output-baseline rewrites claimed as support.
- Reject a wrapper that preserves the old failure mode where
  `emit_prepared_module(...)` reports no authoritative prepared join metadata or
  render contract for the selected `LoadLocal` arm.
