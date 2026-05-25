# AArch64 Prior Preservation Lookup Boundary Checkpoint

Status: Active
Source Idea: ideas/open/02_aarch64_calls_emission_consolidation.md

## Purpose

Continue AArch64 call-emission consolidation after the Step 5 closure review
for the frame-slot call argument boundary narrowing checkpoint rejected
source-idea closure.

## Goal

Remove or narrow the next surviving AArch64-local call-emission boundary that
still reconstructs prior call-preservation, prepared-call lookup, argument
source, byval, dispatch-bridge, or printer authority instead of consuming
shared prepared facts.

## Core Rule

Target-local AArch64 calls code may turn prepared call facts into AArch64
machine nodes and assembly spelling. It must not reconstruct call-plan
authority by scanning retained BIR, walking raw prepared-call lists, or
duplicating argument, byval, preservation, dispatch, or printing decisions
that shared prepared data already owns.

## Latest Closure Review Finding

The Step 5 closure review after the frame-slot call argument boundary narrowing
checkpoint rejects source-idea closure.

The close-time regression guard passed on the existing canonical backend logs:
162/162 tests passed before and after, with no new failures.

The source idea remains open because its durable acceptance criteria are wider
than the completed frame-slot boundary route:

- AArch64 call emission still spans `calls.cpp`, `calls_common.cpp`,
  `calls_argument_sources.cpp`, `calls_byval_aggregates.cpp`,
  `calls_dispatch_bridge.cpp`, `calls_moves.cpp`, `calls_preservation.cpp`,
  `calls_printing.cpp`, and `calls.hpp`.
- `calls.hpp` still exposes broad helpers for argument sources, byval
  aggregates, prepared call lookup, move ordering, preservation lookup,
  preservation republication, and printing.
- `calls_preservation.cpp` still contains prior-preserved-value lookup paths
  that walk prepared call plans and control-flow dominance locally.
- `calls_dispatch_bridge.cpp`, `calls_argument_sources.cpp`,
  `calls_byval_aggregates.cpp`, `calls_moves.cpp`, and `calls_printing.cpp`
  still need boundary review against the source idea's emission-only
  acceptance criteria.
- Closure is not justified until surviving helper files are emission-only,
  printer-owned, dispatch-owned outside this source idea, or explicitly moved
  to the next source idea.

## Read First

- `ideas/open/02_aarch64_calls_emission_consolidation.md`
- `src/backend/mir/aarch64/codegen/calls.hpp`
- `src/backend/mir/aarch64/codegen/calls_preservation.cpp`
- `src/backend/mir/aarch64/codegen/calls_moves.cpp`
- `src/backend/mir/aarch64/codegen/calls_argument_sources.cpp`
- `src/backend/mir/aarch64/codegen/calls_byval_aggregates.cpp`
- `src/backend/mir/aarch64/codegen/calls_dispatch_bridge.cpp`
- `src/backend/mir/aarch64/codegen/calls_printing.cpp`
- Shared prepared-call, value-home, move-bundle, addressing, control-flow, and
  lookup helpers under `src/backend/prealloc/`
- Focused backend call-boundary, prepared-call, preservation, byval,
  aggregate, local-frame, block-entry, AArch64 route, printer, and dispatch
  tests under `tests/backend/mir/`

## Current Targets / Scope

- `calls_preservation.cpp` prior-preserved-value lookup and dominance paths
  first, unless Step 1 proves a smaller adjacent helper boundary is the direct
  blocker.
- `calls.hpp` declarations exposing preservation lookup or broad call-plan
  authority beyond an emission-only surface.
- Adjacent `calls_moves.cpp` consumers only where they call the selected
  preservation helper.
- Other `calls*` files only as evidence for choosing the next checkpoint; do
  not broaden this runbook into a full calls-family rewrite.
- Build metadata only if a helper translation unit is retired.

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
- Do not revisit completed aggregate-address, local-frame publication,
  prior stack-preservation lookup, block-entry republication, or frame-slot
  call argument narrowing routes unless this checkpoint proves one still owns
  duplicate authority.

## Working Model

- Start from `calls_preservation.cpp` and classify each responsibility as
  prepared-owned, AArch64 emission-owned, move-lowering-owned, printer-owned,
  or dispatch-owned.
- Delete or narrow local reconstruction before adding replacement helper
  layers.
- Prefer existing prepared lookup indexes, move bundles, value homes, call
  plans, boundary effects, and control-flow facts when target-local code only
  needs prepared data.
- Keep retained BIR or control-flow checks only for identity validation,
  diagnostics, or emission context that prepared data cannot represent.
- If the selected boundary needs a fact that is genuinely missing, stop and
  record the missing prepared authority in `todo.md` instead of
  reconstructing the decision locally.
- A surviving preservation helper is acceptable only when its parameters and
  callers describe AArch64 emission work.

## Execution Rules

- Start with `calls_preservation.cpp` and preservation-related declarations in
  `calls.hpp`.
- Keep each code slice narrow enough for a fresh build plus focused backend
  proof.
- Escalate to `^backend_` after changing preservation lookup, prepared
  call-plan consumption, move ordering, boundary effects, or call-boundary
  machine instruction emission.
- Reject helper renames, expectation rewrites, and testcase-shaped shortcuts as
  progress.

## Step 1: Select The Preservation Boundary Leak

Goal: identify the concrete preservation helper boundary that still owns
prepared-call lookup, dominance, or prior-preserved-value selection locally.

Primary targets:

- `find_prior_preserved_value_for_call_argument`
- `find_prior_preserved_value_for_value`
- `argument_source_prepared_block_index_by_label`
- `argument_source_prepared_block_successors`
- `prepared_block_dominates`
- preservation lookup declarations in `calls.hpp`
- adjacent `calls_moves.cpp` consumers of these helpers

Actions:

- Map the selected preservation helper responsibility to the source idea's
  acceptance criteria.
- Decide whether the next checkpoint should consume an existing prepared index,
  move the decision to an existing prepared owner, narrow helper parameters, or
  stop on a missing prepared fact.
- Record the selected boundary leak, intended authority owner or missing
  prepared-fact blocker, and focused proof command in `todo.md`.

Completion check:

- `todo.md` names one selected preservation boundary leak, the intended owner,
  and the focused proof scope.

## Step 2: Remove Or Narrow The Selected Boundary

Goal: replace the selected local preservation decision with prepared-fact
consumption or move the helper to the correct existing owner without broadening
the active idea.

Actions:

- Delete the selected local prepared-call reconstruction, prepared-call list
  walk, dominance recheck, or duplicate prior-preserved-value selection.
- Consume existing prepared lookup, move-bundle, value-home, boundary-effect,
  or control-flow facts when sufficient.
- Keep target-local code responsible only for turning selected preservation
  facts into AArch64 machine nodes.
- Tighten helper signatures so obsolete context parameters do not remain.
- Update focused tests only to preserve or strengthen the prepared-authority
  contract.
- Run `cmake --build --preset default` plus the focused backend proof selected
  in Step 1.

Completion check:

- The selected preservation boundary no longer owns duplicate call-planning or
  dominance authority, and `test_after.log` records passing build plus focused
  proof.

## Step 3: Consolidate Helper/API Surface

Goal: remove declarations, helper wrappers, includes, or translation-unit
boundaries made obsolete by Step 2.

Actions:

- Remove obsolete declarations from `calls.hpp`.
- Move or inline helper code back into the smallest surviving emission owner
  when a separate helper no longer describes a real boundary.
- Retire a `calls_*` translation unit only when its remaining code has a clear
  existing owner and build metadata can be updated coherently.
- Preserve AArch64-specific emission details in AArch64 code.
- Run a fresh build and the focused backend proof after each coherent helper
  boundary change.

Completion check:

- The affected helper/API surface is smaller or explicitly emission-only, with
  build metadata and include graphs matching the new boundary.

## Step 4: Broader Backend Checkpoint

Goal: prove the latest preservation-boundary checkpoint did not regress
adjacent call emission, prepared-call, preservation, byval, aggregate,
local-frame, dispatch-bridge, or printer behavior.

Actions:

- Run the supervisor-selected broader backend validation scope.
- Include focused AArch64 call-boundary, prepared-call, preservation,
  argument-source, byval, aggregate, local-frame/publication, dispatch-bridge,
  and printer tests.
- Include affected shared-boundary and x86 tests if shared prepared-call or
  control-flow behavior was touched.
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
