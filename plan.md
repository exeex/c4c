# RISC-V Memory Accesses Stale-Publication Fixture Support

Status: Active
Source Idea: ideas/open/275_riscv_memory_accesses_stale_publication_fixture_support.md

## Purpose

Add supported fixture construction for a RISC-V prepared
`PreparedFunctionLookups::memory_accesses` stale-publication row so a later
bounded fail-closed proof can exercise the real same-consumer backend path
without synthetic mutation or hand-built stale state.

## Goal

Create a supported fixture path where the public prepared `memory_accesses`
row can intentionally differ from the current Route 3 / Route 5 memory-source
identity while preserving the compatible `LoadLocal` publication fixture and
exact `lw a1, 12(s2)` positive output.

## Core Rule

Do not accept direct prepared-state mutation, Route 3 / Route 5 oracle mutation,
lookup-index editing, expectation weakening, or named-case shortcuts as fixture
support. The fixture must use supported preparation APIs and remain tied to the
real RISC-V same-consumer backend path.

## Read First

- `ideas/open/275_riscv_memory_accesses_stale_publication_fixture_support.md`
- `ideas/closed/274_phase_f5_riscv_memory_accesses_stale_publication_fail_closed_proof.md`
- Existing RISC-V dynamic stack-source `LoadLocal` fixture and tests.
- Route 3 / Route 5 same-consumer memory-source agreement code that consumes
  `PreparedFunctionLookups::memory_accesses`.
- Prepared-memory debug, status, printer, wrapper, fallback, and exact-output
  assertions for the same fixture family.

## Current Targets And Scope

- Extend supported RISC-V prepared-fixture construction so one public
  `PreparedFunctionLookups::memory_accesses` row can intentionally be stale
  relative to the current Route 3 / Route 5 memory-source identity.
- Preserve the compatible dynamic stack-source `LoadLocal` fixture byte-for-byte,
  including exact `lw a1, 12(s2)` output.
- Record names or debug/status facts for the prepared memory row, the current
  Route 3 / Route 5 authority row, and the stale public row relationship.
- Leave the final stale-publication fail-closed proof to a later idea unless a
  supervisor explicitly folds that proof into this active runbook.

## Non-Goals

- Do not add the final stale-publication fail-closed proof by default.
- Do not use synthetic post-construction mutation of
  `PreparedFunctionLookups::memory_accesses`, Route 3 / Route 5 oracle rows, or
  lookup indexes as the accepted fixture mechanism.
- Do not broaden this into x86 parity, cross-publication, edge-publication,
  metadata, liveness, aggregate retirement, draft 155 work, or broad Route 3 /
  Route 5 rewrites.
- Do not weaken fallback, status, route-debug, prepared-printer, wrapper,
  baseline, unsupported, or exact-output contracts.

## Working Model

Start from the existing supported RISC-V dynamic stack-source `LoadLocal`
fixture. Identify the construction point where the public prepared
`memory_accesses` row and the Route 3 / Route 5 current memory-source identity
are currently tied together. Add a supported fixture option or helper that can
publish a stale public memory row through normal preparation, while keeping the
compatible fixture stable and making the stale/current relationship visible to
later fail-closed proof work.

## Execution Rules

- Prefer semantic fixture construction over testcase-shaped matching.
- Keep helper-only evidence out of acceptance unless the real RISC-V backend
  consumer path is also exercised.
- Preserve positive fixture output byte-for-byte.
- Make the stale public row and current Route 3 / Route 5 authority row
  distinguishable in test/debug/status evidence.
- Any code-changing step needs fresh build or compile proof plus the
  supervisor-delegated narrow test command.
- Escalate to reviewer or lifecycle repair if the route requires broad Route 3
  / Route 5 rewrites, expectation downgrades, or final fail-closed proof scope.

## Step 1: Locate The Fixture Binding

Goal: Identify where normal RISC-V `LoadLocal` fixture construction binds the
public prepared `memory_accesses` row to the current Route 3 / Route 5
memory-source identity.

Primary target: Existing RISC-V dynamic stack-source `LoadLocal` fixture and
prepared memory publication path.

Actions:

- Locate the supported `LoadLocal` fixture and the helper that publishes
  `PreparedFunctionLookups::memory_accesses`.
- Trace the public prepared row, the current Route 3 / Route 5 authority row,
  and the lookup/index relationship that makes them identical today.
- Identify the narrowest supported construction surface where stale-publication
  fixture support can be added without post-construction mutation.
- Record the named rows, helpers, and constraints in `todo.md`.

Completion check:

- `todo.md` names the current binding point, the compatible public row, the
  current Route 3 / Route 5 authority row, and the intended supported extension
  point.
- No implementation files are changed in this inspection step unless the
  supervisor explicitly delegates Step 2 in the same packet.

## Step 2: Add Supported Stale Fixture Construction

Goal: Extend the fixture construction path so it can intentionally publish a
stale public `memory_accesses` row through supported preparation APIs.

Primary target: Narrow fixture/helper code for the RISC-V same-consumer
`LoadLocal` fixture family.

Actions:

- Add the minimal supported option, helper, or fixture variant needed to express
  the stale public memory row.
- Keep the Route 3 / Route 5 current authority row honest and separately
  identifiable.
- Avoid clearing or directly mutating prepared lookup maps, oracle rows, memory
  edges, or indexes after construction.
- Keep the compatible fixture path as the default or explicitly covered
  positive case.

Completion check:

- A supported fixture path can construct the stale public
  `PreparedFunctionLookups::memory_accesses` row without hand-built stale state.
- The stale row and current Route 3 / Route 5 row are distinguishable by names,
  debug facts, status facts, or assertions.

## Step 3: Prove Compatible And Stale Fixture Coverage

Goal: Add narrow proof that the compatible path remains byte-stable and the new
stale fixture construction path is available for later fail-closed work.

Primary target: Existing RISC-V memory-access fixture tests for the same
consumer path.

Actions:

- Assert the compatible dynamic stack-source `LoadLocal` output remains exactly
  `lw a1, 12(s2)`.
- Assert the supported stale fixture construction path publishes the intended
  stale public memory row and retains the current Route 3 / Route 5 authority
  identity.
- Assert enough debug/status evidence for a later fail-closed proof to consume
  the fixture without synthetic mutation.
- Do not relax unsupported, fallback, status, route-debug, printer, wrapper,
  baseline, or exact-output expectations.

Completion check:

- Narrow backend proof covers both the compatible fixture and the supported
  stale fixture construction path.
- Any changed expectation is justified by real fixture capability, not weaker
  coverage.

## Step 4: Run Acceptance Proof

Goal: Provide fresh proof that fixture support is working and did not weaken
the compatible RISC-V same-consumer path.

Primary target: Supervisor-selected build plus narrow RISC-V memory-access test
subset.

Actions:

- Run the delegated build or compile proof.
- Run the delegated narrow test command for this fixture family.
- If shared Route 3 / Route 5 behavior changed, ask the supervisor to select
  broader validation before accepting the slice.
- Record commands and results in `todo.md`.

Completion check:

- Build or compile proof is fresh.
- Narrow compatible-path and stale-fixture-construction proofs are green.
- No unsupported/status/output/fallback/debug/printer/wrapper/baseline contract
  was weakened.
