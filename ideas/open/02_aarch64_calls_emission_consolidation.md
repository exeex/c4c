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

## Current Lifecycle Blocker

Step 5 closure review after the call move boundary checkpoint rejected closure
and deactivated the active runbook. The checkpoint passed its broader full-suite
monotonic regression proof according to `todo.md` and canonical before/after
logs, but regression proof alone is not sufficient for closure while the
semantic blocker below remains.

The source idea remains open because the remaining call move/source surface in
`calls_moves.cpp`, `calls_argument_sources.cpp`, `calls_byval_aggregates.cpp`,
and `calls.hpp` still depends on target-local caller-side argument source
selection. The completed checkpoint identified a precise remaining blocker:
there is no single prepared call-argument source fact that says a selected
`BeforeCall` argument move should source from prior preservation, local-frame
address materialization, frame-slot/address sources, or byval register-lane
materialization. Until that shared prepared fact exists, AArch64 caller-side
source selection in `lower_before_call_move` is not actionable as another
consolidation checkpoint under this source idea.

Do not reactivate this idea for another AArch64-local move/source cleanup
checkpoint unless either:

- the missing shared prepared call-argument source fact has been introduced, or
- a fresh mapping proves a different remaining helper duplicates an already
  available prepared fact without needing new shared call-plan authority.

## Reviewer Reject Signals

- A patch only concatenates `calls*.cpp` files into a larger file and claims
  architectural progress.
- A patch leaves duplicate call-boundary, byval, preservation, or argument
  placement logic active in both shared and AArch64-local layers.
- A patch moves AArch64 emission details into the shared planner.
- A patch removes diagnostics or behavior without equivalent proof that the
  shared plan now owns the case.
- A patch weakens or deletes tests to make the smaller files look complete.
