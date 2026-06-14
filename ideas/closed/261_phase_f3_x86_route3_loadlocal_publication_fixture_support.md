# 261 Phase F3 x86 Route 3 LoadLocal publication fixture support

## Idea Type

x86 backend testability and supported fixture support.

## Goal

Create a supported x86 fixture path that naturally carries a selected
`LoadLocal` `PreparedEdgePublication::source_memory_access` row into the Route
3 statement-memory agreement facade.

## Why This Exists

Idea 258 added the x86-local agreement facade for selected `LoadLocal`
source-memory identity, but Step 4 could not prove the positive agreement
branch or fail-closed publication rows. Existing x86 fixtures either reject
synthetic publications before the facade, publish only non-`LoadLocal` chains,
or use specialized renderers outside the addressed statement-memory path.

This idea exists so the proof surface can be repaired without overfitting idea
258 with hand-built unsupported `join_transfers` or expectation-only changes.

## Resumed After Idea 262

Step 2 found that the joined-branch route can naturally publish a selected
`LoadLocal` with `source_memory_access`, but x86 rejected the prepared
compare-join shape before it reached
`render_agreed_route3_load_local_statement_memory_operand(...)`. That blocker
was split to idea 262 and is now closed:
`ideas/closed/262_phase_f3_x86_compare_join_loadlocal_selected_arm_support.md`.

Resume this idea at the supported joined-branch route. The next work should
finish the minimal fixture/proof surface for idea 258 using the production
compare-join selected-`LoadLocal` bridge from idea 262.

## Completion Note

Closed after Step 4 recorded that the supported joined-branch compare-join
`EdgeStoreSlot` selected-`LoadLocal` route now provides the proof surface needed
by idea 258. The route naturally carries both the Route 3 `LoadLocal` memory
record and the prepared edge-publication `source_memory_access` facts into the
x86 Route 3 statement-memory agreement facade.

The focused surface covers positive x86 rendering plus the reachable
fail-closed rows recorded during execution: missing source address rejection,
join-carrier-only drift rejection, and incomplete prepared source-memory
publication rejection. Prepared-only, stale-publication, byte-offset drift, and
cross-publication mismatch rows remain out of scope for this fixture because
they require synthetic or stale prepared publication state rather than the
supported route.

Resume `ideas/open/258_phase_f3_x86_route3_loadlocal_source_memory_agreement_bridge.md`
at Step 4 using this joined-branch selected-`LoadLocal` proof surface.

## In Scope

- Identify or add one supported x86 handoff fixture shape that reaches
  `render_agreed_route3_load_local_statement_memory_operand(...)` with both a
  Route 3 `LoadLocal` memory record and a prepared
  `source_memory_access` publication.
- Keep the fixture representative of an allowed backend route rather than a
  synthetic publication injection that an earlier route would reject.
- Add focused tests that prove the positive agreement branch and the rejection
  rows needed by idea 258.
- Preserve existing prepared lookup/status, fallback names, x86 output
  spelling, frame-slot ownership, and route-debug contracts.
- Record how idea 258 can resume once this proof surface exists.

## Out Of Scope

- Rewriting x86 addressing, frame layout, register materialization, or emitted
  output policy.
- Making prepared `memory_accesses` the semantic authority for Route 3
  selected-memory facts.
- Generic memory parity beyond the selected `LoadLocal` source-memory proof
  surface.
- RISC-V behavior changes.
- Downgrading or deleting existing x86 fixture expectations to make room for
  the new proof.

## Acceptance Criteria

- A supported x86 fixture or fixture helper reaches the statement-memory
  agreement facade with a natural selected `LoadLocal`
  `source_memory_access` publication.
- Tests prove positive Route 3/prepared agreement through that supported path.
- Tests prove missing, incomplete, prepared-only, route-only, unsupported, and
  mismatch publication behavior without claiming legacy no-publication fallback
  as positive agreement.
- Existing default prepared tests and x86 route-debug/handoff-boundary tests
  continue to pass.
- `todo.md` records whether idea 258 can be resumed for closure after this
  fixture support lands.

## Reviewer Reject Signals

- Reject hand-built `PreparedEdgePublication` or `join_transfers` rows on an
  x86 route that would reject those rows before reaching the agreement facade.
- Reject tests that prove only legacy no-publication fallback while claiming
  positive Route 3/prepared source-memory agreement.
- Reject named-case shortcuts that match one fixture function, local name, or
  output spelling instead of exercising the supported publication path.
- Reject expectation downgrades, status weakening, helper/oracle renames,
  fallback-name rewrites, or output-baseline rewrites claimed as proof surface
  progress.
- Reject broad target-policy rewrites outside the fixture/testability support
  needed for the selected `LoadLocal` source-memory agreement proof.
