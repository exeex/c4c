# AArch64 Calls Emission Consolidation

## Goal

Shrink and consolidate AArch64 `calls*.cpp` after shared call-plan authority is
available, leaving target-local calls code responsible for emission rather than
planning.

## Why This Exists

The current AArch64 calls family is much larger than the reference target-local
call module because it includes bridge logic, source recovery, move
materialization, preservation, and printing concerns. After shared call-plan
facts exist, those target-local shards should collapse into a small emission
surface that converts prepared call facts into AArch64 machine nodes.

## In Scope

- Merge or delete obsolete `calls_*` translation units once their decision
  logic has moved to shared prepared call planning.
- Keep a small AArch64 call emission owner, likely `calls.cpp`/`calls.hpp`, with
  optional helper files only when they have a clear emission-only boundary.
- Move call printing or effect spelling that is not AArch64 emission into the
  appropriate machine-printer or shared MIR layer.
- Remove redundant argument-source and preservation reconstruction that can be
  read directly from prepared call-plan facts.
- Update build metadata as files are retired.

## Out Of Scope

- Inventing a new call-plan API; that belongs to
  `01_shared_call_plan_authority.md`.
- Broad dispatch cleanup outside call emission.
- Changing behavior solely to reduce line count.
- Large unrelated cleanup in ALU, memory, comparison, variadic, or prologue
  lowering.

## Acceptance Criteria

- AArch64 call emission files are fewer and have clearer responsibilities.
- Target-local calls code does not rederive call-plan decisions already present
  in shared prepared facts.
- Retired files are removed from build metadata and include graphs.
- Focused call tests and a fresh build pass after each coherent consolidation
  slice.
- The line count reduction comes from deleted duplication or moved authority,
  not from hiding behavior in opaque helpers.

## Resolved Lifecycle Blocker

Step 5 closure review after the call move boundary checkpoint rejected closure
and deactivated the active runbook. The checkpoint passed its broader full-suite
monotonic regression proof according to `todo.md` and canonical before/after
logs, but regression proof alone was not sufficient for closure while the
semantic blocker below remained.

The source idea remained open because the remaining call move/source surface in
`calls_moves.cpp`, `calls_argument_sources.cpp`, `calls_byval_aggregates.cpp`,
and `calls.hpp` still depends on target-local caller-side argument source
selection. The completed checkpoint identified a precise remaining blocker:
there is no single prepared call-argument source fact that says a selected
`BeforeCall` argument move should source from prior preservation, local-frame
address materialization, frame-slot/address sources, or byval register-lane
materialization.

That blocker was resolved by
`ideas/closed/01_shared_prepared_call_argument_source_selection.md`.
`PreparedCallArgumentSourceSelection` now provides a shared source-choice fact
for covered selected `BeforeCall` moves, and AArch64 consumes complete shared
selections for frame-slot values, frame-slot addresses, local-frame address
materialization, stack-slot prior preservation, and complete byval
register-lane selections.

Reactivate this idea only through a fresh mapping that proves the next
AArch64-local helper boundary duplicates prepared call-plan facts. The closed
prerequisite does not authorize deleting fallback emission for incomplete
source-selection facts, callee-saved-register prior preservation, or fragmented
byval lanes without another prepared fact or mapping proof.

## Reactivation Note

The reactivation prerequisite mapping was rerun and confirmed the blocker
above was present before the shared prerequisite. The lifecycle was switched to
`ideas/open/01_shared_prepared_call_argument_source_selection.md`, which
implemented and closed the missing shared prepared call-argument
source-selection fact. This consolidation idea is active again for a new
mapping pass.

Historical mapping highlights before the prerequisite:

- `lower_before_call_move` already saw prepared destination and argument-plan
  identity, but chose the selected `BeforeCall` source locally.
- Frame-slot and memory-return sources partially mapped to existing prepared
  argument-plan or memory-return facts, but target-local code constructed the
  selected source path.
- Local-frame address and frame-slot address sources could use prepared address
  materialization when available, but had local fallback selection.
- Byval register-lane sources reconstructed selected source bytes from local
  scans plus prepared memory-access/addressing facts.
- Prior-preserved argument sources could find prepared preserved values, but
  the final choice to source a selected argument move from preservation was
  local.
- Preservation home population and republication already map to prepared
  boundary effects; they are not the remaining selected argument-source
  blocker.

## Reviewer Reject Signals

- A patch only concatenates `calls*.cpp` files into a larger file and claims
  architectural progress.
- A patch leaves duplicate call-boundary, byval, preservation, or argument
  placement logic active in both shared and AArch64-local layers.
- A patch moves AArch64 emission details into the shared planner.
- A patch removes diagnostics or behavior without equivalent proof that the
  shared plan now owns the case.
- A patch weakens or deletes tests to make the smaller files look complete.
