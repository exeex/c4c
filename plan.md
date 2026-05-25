# AArch64 Call Boundary Move Emission Only Runbook

Status: Active
Source Idea: ideas/open/07_aarch64_call_boundary_move_emission_only.md

## Purpose

Reduce AArch64 call-boundary move lowering to target instruction emission from
complete prepared call facts.

## Goal

`calls_moves.cpp` translates prepared source and destination records into
AArch64 operands, effects, and machine instructions without choosing which
semantic call argument source wins.

## Core Rule

Do not move source-selection authority into another AArch64 helper. Any rule
that decides the winning semantic source must live in prepared call planning or
prepared lookup records before emission starts.

## Read First

- `ideas/open/07_aarch64_call_boundary_move_emission_only.md`
- `src/backend/mir/aarch64/codegen/calls_moves.cpp`
- `src/backend/mir/aarch64/codegen/calls_argument_sources.cpp`
- `src/backend/mir/aarch64/codegen/calls_dispatch_bridge.cpp`
- `src/backend/mir/aarch64/codegen/calls.hpp`
- prepared call-plan records and lookup helpers under `src/backend/mir`
- focused call-boundary and instruction-dispatch backend tests

## Current Targets

- Decision logic in `calls_moves.cpp` that still reconstructs prepared facts.
- Structured prepared source and destination records consumed by AArch64 call
  emission.
- Target-local operand, effect, and machine-instruction construction.
- Focused tests for emission from each complete source kind.

## Non-Goals

- Do not change prepared record ownership; ideas `05` and `06` owned that
  authority work.
- Do not retire or consolidate calls files; that belongs to later ideas.
- Do not perform broad dispatch cleanup.
- Do not weaken c_testsuite, dispatch, or call-boundary expectations.
- Do not hide source-selection decisions in printer, effect, or operand
  helpers.

## Working Model

- Prepared planning names the semantic call argument source before AArch64
  emission runs.
- AArch64 call-boundary lowering may inspect structured prepared facts only to
  build target operands and instructions.
- Missing or incomplete prepared facts should fail through an explicit
  diagnostic or rejection path, not through target-local rediscovery.
- Any fallback that remains must be narrow, documented as temporary, and not a
  broad prior-home rederivation path.

## Execution Rules

- Keep code-changing packets small enough for build proof plus focused
  call-boundary or prepared-call tests.
- Preserve existing machine-node payloads and printer behavior unless a payload
  shape change is required to consume prepared facts.
- Add or tighten tests before deleting a fallback when the source kind lacks
  focused coverage.
- Treat named-test shortcuts, expectation weakening, and source-choice
  reconstruction in AArch64 as route failures.
- Escalate to broader validation when prepared record consumption and AArch64
  instruction emission both change in the same slice.

## Ordered Steps

### Step 1: Map Remaining Selection Decisions In Call Moves

Goal: identify every decision in `calls_moves.cpp` that should already be a
prepared fact.

Primary targets:

- `src/backend/mir/aarch64/codegen/calls_moves.cpp`
- `src/backend/mir/aarch64/codegen/calls_argument_sources.cpp`
- `src/backend/mir/aarch64/codegen/calls_dispatch_bridge.cpp`
- prepared call-plan source and destination records

Actions:

- Inspect call-boundary move emission for plan scans, value-home scans,
  preservation lookups, byval lane discovery, frame-slot address recovery, and
  special materialization choices.
- Classify each branch as either target operand construction or semantic source
  selection.
- Identify the prepared record or lookup that should supply each semantic fact.
- Record any missing prepared fact as a blocker for the relevant later step
  rather than solving it with a target-local fallback.

Completion check:

- The executor can name the exact AArch64 branches to keep as emission and the
  exact branches that must be redirected to prepared facts or blocked.

### Step 2: Add Focused Coverage For Complete Source-Kind Emission

Goal: make source-kind coverage explicit before removing target-local
reconstruction.

Primary targets:

- focused prepared-call tests
- AArch64 call-boundary tests
- AArch64 instruction-dispatch tests for call argument moves

Actions:

- Identify source kinds already represented as complete prepared facts.
- Add or tighten tests for register, stack, prior-preserved, byval, frame-slot,
  and materialized argument sources as applicable to current prepared records.
- Keep tests semantic; do not encode named c_testsuite shortcuts.
- Preserve existing printer and machine-node payload expectations.

Completion check:

- Focused tests prove each complete prepared source kind can drive AArch64
  emission without target-local source choice.

### Step 3: Redirect Source Consumption To Prepared Records

Goal: make `calls_moves.cpp` consume prepared source records instead of
reconstructing the winning source.

Primary targets:

- `src/backend/mir/aarch64/codegen/calls_moves.cpp`
- `src/backend/mir/aarch64/codegen/calls_argument_sources.cpp`
- prepared call-plan source-selection APIs

Actions:

- Replace call-plan scans, value-home scans, and preservation source
  rediscovery with structured prepared source consumption.
- Keep AArch64-specific register views, memory operands, immediates, and
  instruction selection local.
- Route incomplete prepared selections to explicit rejection or diagnostics.
- Remove or narrow fallback paths that choose semantic sources during emission.

Completion check:

- AArch64 call-boundary lowering no longer decides which semantic source wins
  for source kinds already covered by prepared records.

### Step 4: Reduce Byval, Frame-Slot, And Materialization Reconstruction

Goal: remove remaining target-local reconstruction that is not pure operand
construction.

Primary targets:

- byval lane and frame-slot address handling in AArch64 calls code
- materialization helpers reached by call-boundary moves
- prepared records for argument storage and materialized values

Actions:

- Move any remaining semantic byval lane, frame-slot, or materialization choice
  to prepared facts when the source idea supports it.
- Leave only target address formation, load/store operand construction, and
  machine-instruction emission in AArch64.
- Document any temporary fallback that cannot be retired in this runbook and
  make it narrow enough for reviewer scrutiny.

Completion check:

- The remaining AArch64 logic for byval, frame-slot, and materialized argument
  paths is emission-oriented and does not rescan raw plans to choose sources.

### Step 5: Validate Emission-Only Boundary

Goal: prove the call-boundary path is behavior-preserving and no longer owns
source selection.

Primary targets:

- focused prepared-call tests
- focused AArch64 call-boundary and instruction-dispatch tests
- representative c_testsuite call cases selected by the supervisor

Actions:

- Run build or compile proof after each code-changing packet.
- Run focused tests for every source kind touched by the slice.
- Run representative backend call tests after fallback removal or source-record
  consumption changes.
- Escalate to supervisor-selected broader validation before treating the
  emission-only boundary as complete.

Completion check:

- Representative dispatch and c_testsuite call paths pass, `calls_moves.cpp`
  consumes complete prepared facts for semantic source choice, and no
  unsupported markings, weaker expectations, or named-case shortcuts were
  introduced.
