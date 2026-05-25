# AArch64 Calls Surviving Boundary Consolidation Checkpoint

Status: Active
Source Idea: ideas/open/02_aarch64_calls_emission_consolidation.md

## Purpose

Continue the AArch64 call-emission consolidation after the Step 5 closure
review for the block-entry preservation republication checkpoint rejected
source-idea closure.

## Goal

Identify and remove the next remaining AArch64-local call-emission boundary
that still rederives prepared call-plan, argument-source, dispatch-bridge,
byval, preservation, or printing decisions instead of consuming shared prepared
facts.

## Core Rule

Target-local AArch64 calls code may turn prepared call facts into AArch64
machine nodes and assembly spelling. It must not reconstruct call-plan
authority by scanning retained BIR, walking raw prepared-call lists, or
duplicating argument, byval, preservation, or dispatch decisions that shared
prepared data already owns.

## Latest Closure Review Finding

The Step 5 closure review after the block-entry preservation republication
checkpoint rejects source-idea closure.

The completed checkpoint removed the selected block-entry republication route's
local call-plan walking and retained-BIR use scan from AArch64 block-entry move
selection. Step 4 backend validation was reported green before this review.

The source idea remains open because its durable acceptance criteria are wider
than the completed block-entry route:

- AArch64 calls are still spread across multiple `calls*.cpp` implementation
  files and a broad `calls.hpp` helper surface.
- `calls_dispatch_bridge.cpp` still mixes dispatch recovery, same-block scalar
  materialization, local aggregate address publication, indirect callee/result
  materialization, and prepared-call bridge work.
- `calls_argument_sources.cpp`, `calls_byval_aggregates.cpp`,
  `calls_moves.cpp`, `calls_preservation.cpp`, and `calls_printing.cpp` still
  need a boundary review against the source idea's emission-only acceptance
  criteria.
- `calls_printing.cpp` remains a possible source-idea target when printing or
  effect spelling is not true AArch64 call emission.
- Closure-time regression guard was not accepted because source-idea
  completion is false; this checkout also did not contain the canonical
  `test_after.log` referenced by the prior Step 4 packet.

## Read First

- `ideas/open/02_aarch64_calls_emission_consolidation.md`
- `src/backend/mir/aarch64/codegen/calls.hpp`
- `src/backend/mir/aarch64/codegen/calls.cpp`
- `src/backend/mir/aarch64/codegen/calls_dispatch_bridge.cpp`
- `src/backend/mir/aarch64/codegen/calls_argument_sources.cpp`
- `src/backend/mir/aarch64/codegen/calls_byval_aggregates.cpp`
- `src/backend/mir/aarch64/codegen/calls_moves.cpp`
- `src/backend/mir/aarch64/codegen/calls_preservation.cpp`
- `src/backend/mir/aarch64/codegen/calls_printing.cpp`
- Shared prepared-call, value-home, move-bundle, addressing, and lookup helpers
  under `src/backend/prealloc/`
- Focused backend call-boundary, prepared-call, byval, aggregate, local-frame,
  block-entry, AArch64 route, printer, and dispatch tests under
  `tests/backend/mir/`

## Current Targets / Scope

- The next AArch64 `calls*` helper boundary that still owns non-emission
  authority.
- `calls_dispatch_bridge.cpp` first, unless Step 1 proves a smaller adjacent
  helper boundary is the direct blocker.
- `calls.hpp` declarations that expose broad call-planning, BIR-recovery, or
  dispatch authority as AArch64 call-emission API.
- `calls_printing.cpp` only when the selected boundary shows printing or effect
  spelling is the next coherent consolidation target.
- Build metadata only when a helper translation unit is retired.

## Non-Goals

- Do not work on `ideas/open/03_dispatch_responsibility_reduction.md`.
- Do not perform broad dispatch cleanup outside AArch64 call emission.
- Do not invent a new shared prepared-call API unless the selected blocker
  proves a required prepared fact is missing.
- Do not move AArch64-specific register, frame-slot, instruction, or assembly
  spelling details into shared planning.
- Do not weaken unsupported or expected-output contracts.
- Do not change behavior solely to reduce line count.
- Do not absorb ALU, memory, comparison, variadic, prologue, or unrelated
  dispatch lowering cleanup into this checkpoint.
- Do not revisit the completed aggregate-address, local-frame publication,
  prior stack-preservation lookup, or block-entry republication routes unless
  this checkpoint proves one still owns duplicate authority.

## Working Model

- Start from a surviving helper boundary and classify each responsibility as
  prepared-owned, AArch64 emission-owned, printer-owned, or dispatch-owned.
- Delete or narrow local reconstruction before adding replacement helper
  layers.
- Prefer existing prepared lookup indexes, move bundles, value homes, call
  plans, and addressing facts when target-local code only needs prepared data.
- Keep retained BIR inspection only for identity validation, diagnostics, or
  emission context that prepared data cannot represent.
- If the selected boundary needs a fact that is genuinely missing, stop and
  record the missing prepared authority in `todo.md` instead of reconstructing
  the decision locally.
- A surviving helper boundary is acceptable only when its parameters and
  callers describe AArch64 emission work.

## Execution Rules

- Start with `calls_dispatch_bridge.cpp` and `calls.hpp` boundary mapping.
- Keep each code slice narrow enough for a fresh build plus focused backend
  proof.
- Escalate to `^backend_` after changing call-boundary effects, prepared lookup
  consumption, byval/aggregate moves, dispatch-bridge call lowering, preserved
  value publication, or printer/effect spelling behavior.
- Reject helper renames, expectation rewrites, and testcase-shaped shortcuts as
  progress.

## Step 1: Select The Next Surviving Boundary Leak

Goal: identify the next concrete AArch64 calls boundary that still owns
prepared-call, BIR-recovery, dispatch-bridge, byval, argument-source,
preservation, or printing authority locally.

Primary targets:

- `calls_dispatch_bridge.cpp` local aggregate address publication, scalar
  materialization, retained-BIR value lookup, indirect callee/result
  materialization, and prior-call lookup paths.
- `calls.hpp` declarations that expose those bridge helpers or preservation
  helpers beyond an emission-only surface.
- Adjacent `calls_argument_sources.cpp`, `calls_byval_aggregates.cpp`,
  `calls_moves.cpp`, `calls_preservation.cpp`, and `calls_printing.cpp`
  boundaries only as needed to choose the smallest coherent checkpoint.

Actions:

- Map surviving helper responsibilities to source-idea acceptance criteria.
- Decide whether the next checkpoint should delete a local decision, move a
  helper to an existing owner, retire a translation unit, or stop on a missing
  prepared fact.
- Record the selected boundary leak, expected replacement owner or missing
  prepared-fact blocker, and focused proof command in `todo.md`.

Completion check:

- `todo.md` names one selected surviving boundary leak, the intended authority
  owner, and the focused proof scope.

## Step 2: Remove Or Narrow The Selected Boundary

Goal: replace the selected local decision with prepared-fact consumption or
move the helper to the correct existing owner without broadening the active
idea.

Actions:

- Delete the selected local prepared-call reconstruction, retained-BIR scan, or
  duplicate argument/byval/preservation/dispatch decision.
- Consume existing prepared call-plan, lookup, move-bundle, value-home,
  addressing, or boundary-effect facts when sufficient.
- Move printer/effect spelling to the printer surface only if Step 1 selects
  that boundary.
- Keep retained BIR checks only for instruction identity validation,
  diagnostics, or unavoidable emission context.
- Tighten helper signatures so obsolete context parameters do not remain.
- Update focused tests only to preserve or strengthen the prepared-authority
  contract.
- Run `cmake --build --preset default` plus the focused backend proof selected
  in Step 1.

Completion check:

- The selected boundary no longer owns duplicate call-planning authority, and
  `test_after.log` records passing build plus focused proof.

## Step 3: Consolidate Helper/API Surface

Goal: remove declarations, helper wrappers, includes, or translation-unit
boundaries made obsolete by Step 2.

Actions:

- Remove obsolete declarations from `calls.hpp`.
- Merge helper code back into the smallest surviving emission owner when a
  separate helper no longer describes a real boundary.
- Retire a `calls_*` translation unit only when its remaining code has a clear
  existing owner and build metadata can be updated coherently.
- Preserve AArch64-specific emission details in AArch64 code.
- Run a fresh build and the focused backend proof after each coherent helper
  boundary change.

Completion check:

- The affected helper/API surface is smaller or explicitly emission-only, with
  build metadata and include graphs matching the new boundary.

## Step 4: Broader Backend Checkpoint

Goal: prove the latest surviving-boundary checkpoint did not regress adjacent
call emission, prepared-call, dispatch bridge, byval, aggregate, preservation,
local-frame, or printer behavior.

Actions:

- Run the supervisor-selected broader backend validation scope.
- Include focused AArch64 call-boundary, dispatch-bridge, prepared-call,
  argument-source, byval, aggregate, local-frame/publication, preservation, and
  printer tests.
- Include affected shared-boundary and x86 tests if shared prepared-call
  behavior was touched.
- Record exact proof commands and results in `todo.md`.

Completion check:

- The broader checkpoint passes, or `todo.md` records a precise blocker tied
  to the active source idea.

## Step 5: Closure Review

Goal: decide whether the source idea is complete after this checkpoint or
whether another runbook checkpoint is needed.

Actions:

- Recheck all surviving `calls*.cpp` and `calls.hpp` boundaries against the
  source idea acceptance criteria.
- Confirm target-local calls code no longer rederives decisions already
  present in shared prepared facts.
- Confirm surviving helper files are emission-only, printer-owned,
  dispatch-owned outside this source idea, or identified as the next checkpoint
  target.
- If durable remaining work exists, keep the idea open and request another
  runbook checkpoint instead of closing.

Completion check:

- Lifecycle state gives the supervisor an unambiguous close/keep-active
  decision.
