# Edge Copy and Block Entry Bookkeeping Helpers

Status: Active
Source Idea: ideas/open/edge-copy-block-entry-bookkeeping-prealloc.md

## Purpose

Move generic Prepared edge-copy and block-entry publication bookkeeping out of
AArch64 dispatch and into prealloc or shared MIR helpers.

Goal: expose a small target-neutral helper for Prepared edge-copy or block-entry
bookkeeping while keeping concrete target move emission and register constraints
in each backend.

## Core Rule

Prealloc or shared MIR may classify and carry Prepared edge-copy/block-entry
bookkeeping; targets still own concrete parallel-copy emission, register
constraints, instruction selection, and target operand construction.

## Read First

- `ideas/open/edge-copy-block-entry-bookkeeping-prealloc.md`
- `src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_publication.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_producers.cpp`
- `src/backend/prealloc/control_flow.hpp`
- `src/backend/prealloc/value_locations.hpp`
- `src/backend/prealloc/prepared_lookups.hpp`
- `src/backend/prealloc/prepared_lookups.cpp`
- `src/backend/prealloc/regalloc.hpp`
- `src/backend/prealloc/regalloc.cpp`
- `src/backend/mir/x86/prepared/prepared.hpp`
- `src/backend/mir/x86/prepared/dispatch.cpp`

## Current Targets

- AArch64 edge-copy dispatch and block-entry publication bookkeeping that
  consumes Prepared move bundles and establishes machine-value state at block
  boundaries.
- Existing prealloc control-flow, value-location, move-bundle, and prepared
  lookup surfaces that can own Prepared fact consumption.
- Shared MIR surfaces only if the selected narrow slice is already a
  target-independent machine-record bookkeeping concern.
- x86 prepared block lowering/query surfaces that can consume the helper
  without duplicating AArch64 move-bundle bookkeeping.

## Non-Goals

- Do not rewrite the complete out-of-SSA pipeline.
- Do not move concrete target move emission, target parallel-copy instruction
  selection, register constraints, or target operand construction into generic
  helpers.
- Do not combine this with call-boundary classification, formal-entry
  publication, or operand decoding migrations.
- Do not weaken branch/control-flow tests or reclassify supported paths as
  unsupported.
- Do not add testcase-shaped matching for one block label, branch shape, or
  named edge.

## Working Model

- Prepared move bundles and out-of-SSA edge-transfer facts already live before
  target codegen.
- Prealloc is the best destination when the helper consumes Prepared facts such
  as move bundles, edge-transfer records, or block-entry publication authority.
- Shared MIR is only appropriate after the data has become target-independent
  machine-record bookkeeping.
- AArch64 should consume the helper and retain concrete move emission, register
  selection, instruction records, and diagnostics locally.
- x86 prepared block lowering should be able to query or consume the same helper
  when it lowers Prepared move bundles.

## Execution Rules

- Start with inventory and choose exactly one narrow ownership boundary before
  adding helper APIs.
- Keep each implementation step behavior-preserving unless the source idea
  explicitly requires a boundary change.
- Prefer a small helper/record extraction over broad control-flow or
  out-of-SSA rewrites.
- Keep target-specific move legality and emission visibly local.
- Treat expectation downgrades, unsupported reclassification, named-edge
  shortcuts, duplicate unused helpers, or unclear prealloc/shared-MIR ownership
  as route failure.
- After code-changing steps, run a fresh build or compile proof plus the
  delegated narrow test subset.
- Before final acceptance, validate at least two nearby control-flow shapes,
  such as a simple branch/merge and a case with multiple incoming edge
  publications where currently supported.

## Ordered Steps

### Step 1: Inventory Edge-Copy And Block-Entry Bookkeeping

Goal: identify the smallest target-neutral bookkeeping unit currently owned by
AArch64 and decide whether its proper destination is prealloc or shared MIR.

Primary targets:

- `src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_publication.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_producers.cpp`
- `src/backend/prealloc/control_flow.hpp`
- `src/backend/prealloc/value_locations.hpp`
- `src/backend/prealloc/prepared_lookups.hpp`

Actions:

- Inspect AArch64 edge-copy and block-entry paths that read Prepared move
  bundles, edge-transfer facts, value-location facts, and publication records.
- Classify each branch as target-neutral Prepared bookkeeping, shared
  machine-record bookkeeping, concrete AArch64 move emission, or diagnostics.
- Choose one narrow first helper boundary and one destination layer: prealloc
  for Prepared fact consumption, or shared MIR for machine-record bookkeeping.
- Record in `todo.md` the chosen boundary, destination rationale, extraction
  target, and target-local responsibilities that must remain in AArch64.

Completion check:

- `todo.md` names the selected narrow helper boundary, gives a clear
  prealloc/shared-MIR ownership rationale, and separates bookkeeping from
  target move emission.

### Step 2: Add The Shared Bookkeeping Helper

Goal: introduce the selected prealloc or shared MIR helper without depending on
AArch64 target types.

Primary targets:

- `src/backend/prealloc/control_flow.hpp`
- `src/backend/prealloc/value_locations.hpp`
- `src/backend/prealloc/prepared_lookups.hpp`
- `src/backend/prealloc/prepared_lookups.cpp`
- New prealloc or shared MIR helper files only if existing surfaces become too
  crowded.
- Focused backend/prealloc or MIR tests under `tests/backend` if a direct
  harness can exercise the helper.

Actions:

- Define helper records or query functions for the selected target-neutral
  bookkeeping unit.
- Preserve access to source Prepared facts needed by target-specific final move
  emission.
- Keep helper names independent of AArch64 terminology.
- Add direct focused coverage if the helper can be tested without target
  emission.
- Run a build or compile proof for the new API surface.

Completion check:

- The selected layer exposes reusable edge-copy or block-entry bookkeeping
  without depending on AArch64 registers, operands, instruction records, or move
  emission.

### Step 3: Adapt AArch64 To Consume The Shared Helper

Goal: make AArch64 edge-copy or block-entry dispatch consume the shared helper
while keeping concrete target move emission local.

Primary targets:

- `src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_publication.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_producers.cpp`
- Existing AArch64 control-flow, branch, or publication tests under
  `tests/backend`

Actions:

- Replace target-neutral raw Prepared bookkeeping in AArch64 with calls to the
  shared helper.
- Keep concrete target moves, register constraints, machine instruction
  records, target operands, and emission diagnostics in AArch64 code.
- Preserve unsupported-form behavior at least as strictly as before.
- Prove unchanged AArch64 behavior for affected branch/merge or edge-copy
  shapes.

Completion check:

- The diff shows ownership movement rather than a thin wrapper around old
  AArch64 helper names, and AArch64 edge-copy/block-entry tests still pass
  without weakened expectations.

### Step 4: Prove Reuse Path For x86 Prepared Blocks

Goal: document or wire a concrete x86 prepared block-lowering consumer for the
shared helper without expanding into a full x86 control-flow rewrite.

Primary targets:

- `src/backend/mir/x86/prepared/prepared.hpp`
- `src/backend/mir/x86/prepared/dispatch.cpp`
- Existing x86 prepared or handoff tests under `tests/backend/bir`

Actions:

- Identify the x86 prepared block-lowering or query point that should consume
  the shared helper.
- Add a narrow query, type use, adapter, or nearby code note showing how x86 can
  consume the helper.
- Keep x86 target move emission, register classes, operand spelling, and
  instruction selection local to x86.
- Add or update focused tests if the reuse path can be proved without a full
  x86 lowering rewrite.

Completion check:

- There is concrete code or test evidence that x86 can reuse the shared
  edge-copy/block-entry bookkeeping instead of duplicating AArch64 move-bundle
  consumption.

### Step 5: Validate Behavior And Anti-Overfit Coverage

Goal: prove the boundary move did not change supported behavior or narrow the
control-flow/move contract.

Actions:

- Run the supervisor-delegated build or compile proof.
- Run narrow backend/prepared tests covering at least two nearby control-flow
  shapes, such as a simple branch/merge and a case with multiple incoming edge
  publications where currently supported.
- Include x86 reuse proof if Step 4 added a query or adapter.
- Compare before/after output or logs where available to show unchanged AArch64
  lowering.
- Escalate to broader backend validation if multiple shared control-flow or
  target block-lowering surfaces changed.

Completion check:

- `test_after.log` or the delegated proof artifact records passing validation
  over multiple control-flow or edge-publication shapes.
- No tests are weakened or reclassified to make the route pass.
- `todo.md` includes the x86 reuse note required by the source idea.

## Final Acceptance

- One target-neutral edge-copy or block-entry bookkeeping unit lives in
  prealloc or shared MIR with clear ownership rationale.
- AArch64 still owns concrete target move emission, register constraints,
  target operands, instruction selection, and diagnostics.
- x86 has a concrete reuse path for the shared helper.
- Proof covers at least two nearby control-flow shapes and does not rely on a
  single named block, edge, or testcase.
