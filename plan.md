# Shared CFG, Branch, Join, And Loop Materialization Before X86 Emission Runbook

Status: Active
Source Idea: ideas/open/58_bir_cfg_and_join_materialization_for_x86.md
Activated from: ideas/open/58_bir_cfg_and_join_materialization_for_x86.md

## Purpose

Turn idea `58` into an execution runbook that moves branch, join, and
loop-carried control-flow meaning into one authoritative shared prepared
contract before x86 scalar lowering.

## Goal

Land explicit prepared control-flow semantics that shared code owns and x86
consumes, so ordinary branches, joins, short-circuit flow, and loop headers no
longer depend on x86 whole-shape CFG recovery.

## Core Rule

Do not claim progress by widening x86 matcher families or downgrading tests.
Every accepted packet must strengthen shared control-flow ownership first and
show x86 consuming that ownership second.

## Read First

- [ideas/open/58_bir_cfg_and_join_materialization_for_x86.md](/workspaces/c4c/ideas/open/58_bir_cfg_and_join_materialization_for_x86.md)
- [src/backend/prealloc/prealloc.hpp](/workspaces/c4c/src/backend/prealloc/prealloc.hpp)
- [src/backend/prealloc/prealloc.cpp](/workspaces/c4c/src/backend/prealloc/prealloc.cpp)
- [src/backend/prealloc/legalize.cpp](/workspaces/c4c/src/backend/prealloc/legalize.cpp)
- [src/backend/backend.cpp](/workspaces/c4c/src/backend/backend.cpp)
- [src/backend/mir/x86/codegen/prepared_module_emit.cpp](/workspaces/c4c/src/backend/mir/x86/codegen/prepared_module_emit.cpp)
- [tests/backend/backend_lir_to_bir_notes_test.cpp](/workspaces/c4c/tests/backend/backend_lir_to_bir_notes_test.cpp)
- [tests/backend/backend_x86_handoff_boundary_test.cpp](/workspaces/c4c/tests/backend/backend_x86_handoff_boundary_test.cpp)

## Current Targets

- prepared control-flow contract surfaces under
  `src/backend/prealloc/*` and adjacent shared backend headers
- shared production of branch-condition and join-transfer semantics
- x86 prepared-module consumption in
  `src/backend/mir/x86/codegen/prepared_module_emit.cpp`
- contract-focused tests under `tests/backend/*`

## Non-Goals

- idea `60` value-home, frame-layout, or move-resolution ownership beyond the
  semantic boundary this idea must define
- idea `59` generic scalar instruction selection beyond the control-flow
  contract it depends on
- backend-local matcher growth as a temporary proof strategy
- expectation rewrites that weaken supported-path guarantees

## Working Model

- shared code must publish one explicit prepared representation for branch
  condition semantics before x86 lowering
- shared code must publish one explicit prepared representation for former phi
  and join-transfer semantics before x86 lowering
- loop-carried updates and short-circuit control flow must travel through the
  same contract family rather than backend-local special cases
- x86 may still choose target instruction sequences, but it must not recover
  semantic meaning from whole-CFG shape to do so

## Execution Rules

- prefer contract work in shared code before consumer cleanup in x86
- keep semantic transfer ownership distinct from physical move and location
  ownership
- choose proving slices that remove or bypass one ordinary x86 matcher
  dependency as evidence the contract is real
- validate each code packet as `build -> narrow contract seam tests`, and
  escalate to broader x86 backend coverage when the shared contract changes
  across multiple control-flow families
- if execution reveals a separate initiative, record it under `ideas/open/`
  instead of stretching this runbook

## Step 1. Pin Down The Authoritative Control-Flow Contract

Goal: Decide the exact prepared structures and invariants that will represent
branch-condition and join-transfer semantics.

Primary targets:
- [src/backend/prealloc/prealloc.hpp](/workspaces/c4c/src/backend/prealloc/prealloc.hpp)
- [src/backend/prealloc/prealloc.cpp](/workspaces/c4c/src/backend/prealloc/prealloc.cpp)
- [src/backend/prealloc/legalize.cpp](/workspaces/c4c/src/backend/prealloc/legalize.cpp)

Actions:
- inspect the current prepared data model and legalization handoff
- choose where explicit prepared condition semantics live
- choose where explicit join-transfer semantics live
- document the minimal consumer-facing invariants in headers or adjacent shared
  contract comments

Completion check:
- one explicit shared contract surface is named for branch conditions and one
  for join transfers, with consumer-visible invariants clear enough for x86
  to rely on them

## Step 2. Produce The Contract In Shared Lowering

Goal: Make shared preparation create the chosen control-flow semantics for
ordinary branches, joins, short-circuit flow, and loop-carried updates.

Primary targets:
- [src/backend/prealloc/legalize.cpp](/workspaces/c4c/src/backend/prealloc/legalize.cpp)
- [src/backend/prealloc/prealloc.cpp](/workspaces/c4c/src/backend/prealloc/prealloc.cpp)
- any adjacent shared backend owner needed to preserve clean layering

Actions:
- generate the explicit prepared condition and join-transfer records
- ensure former phi meaning survives through the shared contract rather than
  implicit slot traffic
- ensure loop-header and short-circuit families use the same contract family
  instead of x86-local recovery

Completion check:
- prepared output carries authoritative shared control-flow semantics for at
  least one vertical slice that includes branches, joins, and a loop- or
  short-circuit-adjacent case

## Step 3. Switch X86 To Consume The Shared Contract

Goal: Update the prepared x86 path so ordinary control-flow lowering uses the
shared contract instead of whole-shape recovery.

Primary targets:
- [src/backend/mir/x86/codegen/prepared_module_emit.cpp](/workspaces/c4c/src/backend/mir/x86/codegen/prepared_module_emit.cpp)
- any nearby x86 helper that currently reconstructs branch or join meaning

Actions:
- replace one ordinary matcher-driven path with contract-driven consumption
- remove or de-authorize the corresponding whole-shape recovery logic where
  the shared contract makes it unnecessary
- keep target-local work limited to legal x86 spelling choices

Completion check:
- x86 lowers the chosen branch/join family by consuming explicit prepared
  semantics, and at least one prior whole-shape dependency is gone or no
  longer authoritative

## Step 4. Prove The Contract Seam

Goal: Show the new route is real at the shared handoff boundary rather than a
narrow matcher rename.

Actions:
- run `cmake --build --preset default`
- run
  `ctest --test-dir build -R '^(backend_lir_to_bir_notes|backend_x86_handoff_boundary(_test)?)$' --output-on-failure`
- when shared contract coverage expands across multiple control-flow families,
  also run
  `ctest --test-dir build -L x86_backend --output-on-failure`
- record the exact proof and the contract family advanced in `todo.md`

Completion check:
- tests assert contract-level behavior, narrow proof passes, and broader x86
  backend coverage is added when the slice changes multiple control-flow
  families

## Step 5. Check Remaining Matcher Pressure

Goal: Decide whether the current runbook should continue with another contract
consumer slice or return for a lifecycle checkpoint.

Actions:
- compare the remaining x86 matcher families against the shared contract now
  available
- record the next honest packet in `todo.md` by owner surface and semantic gap
- request lifecycle repair only if the runbook no longer matches the source
  idea or a separate initiative is discovered

Completion check:
- the next packet is described as shared-contract expansion or x86 contract
  consumption, not as testcase-shaped matcher growth
