# Dispatch Value Materialization Authority Runbook

Status: Active
Source Idea: ideas/open/12_dispatch_value_materialization_authority.md

## Purpose

Start dispatch cleanup by moving value materialization decisions out of generic
AArch64 dispatch traversal and into clearer owner surfaces.

## Goal

Value materialization has a named owner separate from generic dispatch,
`dispatch.cpp` is easier to read as routing/traversal code, and existing
AArch64 materialization behavior stays fixed.

## Core Rule

Separate responsibility without changing lowering semantics. Do not fold in
edge-copy cleanup, publication cleanup, calls cleanup, broad pipeline redesign,
unsupported downgrades, or testcase-shaped materialization shortcuts.

## Read First

- `ideas/open/12_dispatch_value_materialization_authority.md`
- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_value_materialization.hpp`
- `src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp`
- Direct clients of `dispatch_value_materialization.hpp`
- Focused backend tests covering AArch64 instruction dispatch, target records,
  machine printing, and materialized values feeding later operations

## Current Targets

- Value materialization helpers currently declared in
  `dispatch_value_materialization.hpp`.
- Direct materialization helpers called from `dispatch.cpp`,
  `dispatch_publication.cpp`, `dispatch_edge_copies.cpp`, and
  `calls_dispatch_bridge.cpp`.
- Includes and public declarations that make materialization look like generic
  dispatch authority.
- Focused backend proof for materialized values that feed later AArch64
  operations.

## Non-Goals

- Do not redesign edge-copy, publication, or control-flow ownership.
- Do not reopen completed calls cleanup.
- Do not replace the AArch64 codegen pipeline.
- Do not move semantic authority into vague buckets such as another broad
  dispatch helper.
- Do not reintroduce text-emission strings as an internal semantic contract.
- Do not weaken tests, expectation contracts, or unsupported classifications.

## Working Model

- `dispatch.cpp` should route prepared/BIR instructions through traversal and
  target lowering, not own value materialization policy.
- Some helpers in `dispatch_value_materialization.*` may already be correctly
  materialization-owned; others may belong to prepared facts, shared MIR
  utilities, publication, memory, ALU/float/cast lowering, globals, or target
  emission.
- Ownership moves should happen only after an audit names each helper's current
  clients and true owner.
- Behavior-preserving moves are acceptable; semantic capability changes require
  a separate source idea unless they are strictly required to preserve existing
  materialization behavior.

## Execution Rules

- Audit and classify before moving declarations or files.
- Keep each packet narrow enough to pair with build proof and a focused backend
  subset selected by the supervisor.
- Prefer named owner surfaces over generic dispatch-adjacent names.
- Update includes, declarations, and build metadata in the same packet as the
  ownership move that requires them.
- Preserve existing machine instruction records, printer output, and prepared
  fact usage.
- Treat expectation rewrites, unsupported downgrades, named-test shortcuts, and
  text-string matching as route failures.
- Escalate to broader backend validation before closure because dispatch
  materialization has cross-feature blast radius.

## Ordered Steps

### Step 1: Audit Materialization Helpers and Clients

Goal: classify current value materialization helpers by true owner before any
code move.

Primary targets:

- `src/backend/mir/aarch64/codegen/dispatch_value_materialization.hpp`
- `src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp`
- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- direct includes/callers of `dispatch_value_materialization.hpp`

Actions:

- List each public helper in `dispatch_value_materialization.hpp`, its
  implementation location, and its direct clients.
- Classify each helper as prepared-fact lookup, shared MIR utility,
  publication/materialization bridge, target emission, ALU/float/cast lowering,
  globals/address materialization, or dispatch traversal support.
- Identify helpers that can stay behind a materialization owner and helpers
  that should move to narrower existing owners.
- Identify focused tests that already cover each helper family and gaps that
  need new focused coverage.

Completion check:

- The executor can name the ownership target for each helper family, the first
  safe move packet, and the focused proof subset for that packet without
  re-deriving the audit.

### Step 2: Establish the Named Materialization Owner

Goal: make value materialization authority explicit and separate from generic
dispatch traversal.

Primary targets:

- materialization-owned declarations from Step 1
- `dispatch_value_materialization.*` or its renamed/reframed successor
- includes in dispatch-adjacent translation units

Actions:

- Rename, split, or reframe the materialization owner only if the audit shows
  the current name/surface still implies generic dispatch ownership.
- Keep helpers that genuinely own target value materialization together under
  the named owner.
- Move private implementation details out of the public header where no
  external client requires them.
- Update includes and build metadata for the owner change.
- Run build proof and the supervisor-selected focused backend subset.

Completion check:

- A reader can identify the materialization owner without reading
  `dispatch.cpp`, public declarations match real clients, and focused proof
  shows behavior is unchanged.

### Step 3: Move Misowned Helpers to Existing Narrow Owners

Goal: remove helpers from the materialization surface when their true owner is
  already an existing narrower codegen domain.

Primary targets:

- helpers classified as ALU, float, cast, memory, globals/address, publication,
  or prepared-fact lookup in Step 1
- affected headers and clients
- build metadata affected by moved definitions

Actions:

- Move each helper to the existing owner named by the audit, keeping file moves
  behavior-preserving.
- Avoid creating a new umbrella helper for unrelated materialization-adjacent
  behavior.
- Keep source-level facts structured; do not replace them with printed text or
  named-test matching.
- Update callers to include the narrower owner header.
- Run build proof and focused tests for every moved helper family.

Completion check:

- Misowned helpers no longer appear on the materialization surface, clients use
  narrower owner headers, and focused tests covering moved helper families pass.

### Step 4: Thin `dispatch.cpp` Materialization Touchpoints

Goal: leave `dispatch.cpp` focused on traversal and routing after authority
  moves.

Primary targets:

- direct materialization logic inside `dispatch.cpp`
- dispatch includes made stale by earlier ownership moves
- call sites that can delegate to the named materialization owner or narrower
  helper owners

Actions:

- Replace remaining direct materialization decisions in `dispatch.cpp` with
  calls to the owner surface selected in earlier steps.
- Remove stale includes and local helper declarations from `dispatch.cpp`.
- Preserve routing order, diagnostics, instruction records, and emitted target
  behavior.
- Run build proof and focused AArch64 instruction dispatch/materialization
  tests.

Completion check:

- `dispatch.cpp` no longer hides value materialization decisions, remaining
  materialization call sites are explicit delegations, and focused proof stays
  green.

### Step 5: Add or Tighten Focused Materialization Coverage

Goal: prove materialized values feeding later AArch64 operations stay fixed
  under the new ownership boundary.

Primary targets:

- focused AArch64 backend tests selected from the Step 1 coverage audit
- materialized values feeding ALU, float, cast, memory, globals/address,
  publication, edge-copy, or call-bridge operations as applicable

Actions:

- Add focused tests only where existing coverage does not prove the moved
  materialization path.
- Prefer semantic facts and machine instruction records over brittle assembly
  text matching when the test can observe structured output.
- Keep tests general to the materialization path; do not name-match one narrow
  testcase shape.
- Run build proof and the focused backend subset selected by the supervisor.

Completion check:

- Focused tests cover materialized values feeding later AArch64 operations, no
  contracts are weakened, and the new owner boundary is covered by green proof.

### Step 6: Validate Dispatch Materialization Cleanup

Goal: prove the authority cleanup preserved behavior across the affected
  backend surface.

Primary targets:

- build proof
- focused AArch64 dispatch/materialization tests
- broader backend subset selected by the supervisor

Actions:

- Run build proof after the final ownership packet.
- Run focused tests covering dispatch routing, materialization helpers,
  target instruction records, and machine-printer materialization paths.
- Escalate to the supervisor-selected broader backend validation before
  treating the idea as closure-ready.
- Review the diff for hidden edge-copy/publication/calls cleanup, expectation
  weakening, text-contract regressions, or testcase-shaped shortcuts.

Completion check:

- Build proof and focused/broader validation pass, `dispatch.cpp` is traversal
  focused, value materialization has a named owner, and no out-of-scope cleanup
  or test weakening is mixed into the slice.
