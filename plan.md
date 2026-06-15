# Phase F5 RISC-V Memory Accesses Stale-Publication Fail-Closed Proof

Status: Active
Source Idea: ideas/open/274_phase_f5_riscv_memory_accesses_stale_publication_fail_closed_proof.md

## Purpose

Prove one bounded RISC-V same-consumer stale-publication
`PreparedFunctionLookups::memory_accesses` boundary row fails closed instead of
being accepted as Route 3 / Route 5 semantic agreement.

## Goal

Exercise a real supported RISC-V dynamic stack-source `LoadLocal` backend
consumer path, preserve its compatible positive output, and show one stale
prepared `memory_accesses` fact is rejected fail-closed.

## Core Rule

Do not expand this into the full draft 274 backlog. This runbook owns one
RISC-V stale-publication proof row and must not weaken expected output,
fallback, helper/oracle status, route-debug, prepared-printer, wrapper, or
baseline contracts.

## Read First

- `ideas/open/274_phase_f5_riscv_memory_accesses_stale_publication_fail_closed_proof.md`
- Existing RISC-V dynamic stack-source `LoadLocal` fixture and tests.
- Route 3 / Route 5 agreement code that consumes
  `PreparedFunctionLookups::memory_accesses`.
- Current prepared-memory debug, printer, wrapper, and fallback assertions for
  the same-consumer fixture.

## Current Targets And Scope

- Target only the supported RISC-V dynamic stack-source `LoadLocal` fixture if
  normal construction can express a stale-publication memory row.
- Name the prepared memory row, the stale Route 3 / Route 5 authority relation,
  the expected fail-closed status, and the stable positive output.
- Prove an old or stale prepared memory fact does not become semantic agreement
  for the real backend consumer.
- Keep the positive same-consumer fixture byte-stable, including exact RISC-V
  load output.

## Non-Goals

- Do not claim x86 public-field parity.
- Do not claim exhaustive byte-offset, cross-publication, edge-publication,
  metadata, liveness, aggregate retirement, or draft 155 readiness.
- Do not demote, delete, privatize, wrap, or retire prepared aggregate APIs.
- Do not move target policy into BIR; stack, storage, addressing, source-home,
  register, layout, wrapper, formatting, emission, instruction spelling, and
  exact output remain target-owned.
- Do not use testcase-shaped shortcuts, named-case-only branches, or synthetic
  stale state that normal construction would reject.

## Working Model

The proof should begin from the existing supported same-consumer RISC-V
`LoadLocal` path. First identify whether that path can publish a stale
`memory_accesses` row through normal construction. If yes, add the narrow
negative proof and keep the compatible positive output unchanged. If no,
close this runbook as a blocker and create a narrower fixture-support follow-up
idea under `ideas/open/`.

## Execution Rules

- Prefer semantic boundary checks over testcase-shaped matching.
- Treat helper-only evidence as insufficient unless the real backend consumer
  path is also exercised.
- Preserve positive fixture output byte-for-byte.
- Record failure evidence that distinguishes stale-publication rejection from
  helper-only behavior.
- Any implementation step needs fresh build or compile proof plus the
  supervisor-delegated narrow test command.
- Escalate to reviewer or lifecycle repair if the proof requires broad Route 3
  / Route 5 rewrites or fixture support outside this idea.

## Step 1: Inspect Fixture Expressiveness

Goal: Determine whether the supported RISC-V dynamic stack-source `LoadLocal`
fixture can express the stale-publication `memory_accesses` row through normal
construction.

Primary target: Existing RISC-V same-consumer `LoadLocal` fixture and its
prepared memory publication path.

Actions:

- Locate the supported RISC-V dynamic stack-source `LoadLocal` fixture.
- Trace how the fixture publishes and consumes
  `PreparedFunctionLookups::memory_accesses`.
- Identify the prepared memory row and the Route 3 / Route 5 authority relation
  that would be stale.
- Decide whether stale publication is constructible without synthetic state or
  named-case shortcuts.

Completion check:

- `todo.md` records the fixture path, the named row/relation, and whether
  normal construction can express the stale-publication row.
- If unsupported, execution stops with a fixture-support follow-up idea request
  instead of forcing a synthetic stale row.

## Step 2: Add The Bounded Fail-Closed Proof

Goal: Prove the stale prepared `memory_accesses` fact fails closed on the real
RISC-V same-consumer backend path.

Primary target: Narrow test or fixture code for the existing RISC-V
same-consumer path.

Actions:

- Add the stale-publication case only if Step 1 found normal fixture support.
- Assert the expected fail-closed status and debug facts.
- Assert that Route 3 / Route 5 semantic agreement is not accepted for the
  stale prepared memory fact.
- Keep the proof tied to the real backend consumer, not only helper formatting
  or classification.

Completion check:

- The narrow proof fails before the capability fix or directly verifies the
  corrected fail-closed behavior after the fix.
- Failure evidence records enough status/debug facts to distinguish
  stale-publication rejection from helper-only behavior.

## Step 3: Preserve The Compatible Positive Path

Goal: Prove the compatible same-consumer path remains byte-stable.

Primary target: Existing positive RISC-V output assertions for the same
fixture family.

Actions:

- Keep exact expected RISC-V load output for the compatible fixture.
- Preserve helper/oracle status, fallback behavior, route-debug output,
  prepared-printer output, wrapper output, and baseline behavior.
- Do not relax output or status expectations to make the stale case pass.

Completion check:

- The positive same-consumer fixture still passes with exact output and status
  contracts unchanged.
- Any changed expectation is justified by real capability repair, not by weaker
  coverage.

## Step 4: Run Acceptance Proof

Goal: Provide fresh proof that the bounded stale-publication row fails closed
and the compatible positive path remains stable.

Primary target: Supervisor-selected build plus narrow RISC-V memory-access test
subset.

Actions:

- Run the delegated build or compile proof.
- Run the delegated narrow test command for this fixture family.
- If the implementation touched shared Route 3 / Route 5 behavior, ask the
  supervisor to select broader validation before accepting the slice.
- Record commands and results in `todo.md`.

Completion check:

- Build or compile proof is fresh.
- Narrow stale-publication proof and compatible positive path are green.
- No unsupported/status/output/fallback/debug/printer/wrapper/baseline contract
  was weakened.
