# Dispatch Responsibility Reduction Runbook

Status: Active
Source Idea: ideas/open/03_dispatch_responsibility_reduction.md

## Purpose

Make AArch64 dispatch read as prepared block traversal plus instruction and
terminator routing, not as a mixed owner for materialization, publication,
edge copies, producer lookup, diagnostics, and call-specific glue.

## Goal

Reduce `dispatch*` responsibilities by moving non-routing behavior behind
clear owner surfaces while preserving the compiled target MIR / machine-node
route documented in `src/backend/mir/aarch64/codegen/README.md`.

## Core Rule

Keep each step behavior-preserving unless the source idea explicitly requires a
semantic ownership change; do not use mechanical renames as evidence of
progress.

## Read First

- `ideas/open/03_dispatch_responsibility_reduction.md`
- `src/backend/mir/aarch64/codegen/README.md`
- AArch64 dispatch implementation files under
  `src/backend/mir/aarch64/codegen/`
- Existing materialization, publication, edge-copy, producer lookup, and call
  plan helpers touched by dispatch

## Current Scope

- Define `dispatch.cpp` as the owner for prepared block traversal and routing.
- Move value materialization and publication behind named owner surfaces.
- Separate edge-copy or join-value handling where the current code supports
  that split.
- Remove or justify call-specific dispatch glue only when shared call-plan
  facts already cover the case.
- Preserve focused backend proof plus fresh build proof after each code slice.

## Non-Goals

- Do not replace the whole AArch64 codegen pipeline.
- Do not reintroduce text-first emission or parse printed assembly as an
  internal contract.
- Do not create markdown shards under `src/backend/mir/aarch64/codegen/`.
- Do not expand target-local compatibility matching for narrow prepared BIR
  shapes.
- Do not resume the blocked AArch64-local call move/source consolidation from
  `ideas/open/02_aarch64_calls_emission_consolidation.md` unless the missing
  shared prepared call-argument source fact exists or a fresh mapping proves a
  different helper duplicates an already available prepared fact.

## Working Model

- Dispatch should decide where each prepared block, instruction, or terminator
  is routed.
- Materialization owners should create target machine values from prepared
  facts or target-local lowering facts.
- Publication owners should record produced values and machine-node homes.
- Edge-copy or join-value owners should handle control-flow transfer copies
  separately from ordinary instruction dispatch when the existing model allows
  it.
- Same-block producer recovery should move behind a concrete owner name rather
  than remain implicit dispatch responsibility.

## Execution Rules

- Start each implementation step by mapping current call sites and ownership
  boundaries before editing code.
- Prefer small, buildable extractions over broad rewrites.
- Retain diagnostics unless an equivalent owner now reports the same failure.
- Update build metadata whenever files are split, added, or retired.
- For every code-changing step, run a fresh build or compile proof and the
  focused backend subset selected by the supervisor.
- Escalate to broader validation when a step changes shared ownership,
  cross-file interfaces, or multiple dispatch responsibility buckets.

## Step 1: Map Dispatch Responsibilities

Goal: establish the concrete responsibility map and first extraction target.

Primary targets:

- AArch64 `dispatch*` implementation files
- `src/backend/mir/aarch64/codegen/README.md`
- Nearby materialization, publication, edge-copy, and producer helper files

Actions:

- Inspect the current dispatch files and list the routines that perform
  traversal/routing versus materialization, publication, edge copies,
  same-block producer lookup, diagnostics, and call-specific glue.
- Identify one small extraction target whose owner name is clearer than
  dispatch and whose proof can stay focused.
- Confirm whether any candidate touches the blocked call-argument source
  selection from `ideas/open/02_aarch64_calls_emission_consolidation.md`; defer
  that candidate if it does.

Completion check:

- `todo.md` records the selected first extraction target, excluded blocked-call
  areas, and the supervisor-selected proof command for the executor packet.

## Step 2: Extract Value Materialization Ownership

Goal: move non-routing value materialization out of dispatch behind an explicit
owner surface.

Actions:

- Extract the selected value materialization logic into a named helper or file
  that reflects its real ownership.
- Keep dispatch call sites focused on routing into that owner.
- Preserve existing diagnostics and behavior.
- Update build metadata when the extraction creates, removes, or renames
  files.

Completion check:

- Dispatch no longer owns the extracted materialization details.
- Focused backend proof and fresh build proof pass.
- `todo.md` records proof commands and any remaining nearby materialization
  candidates.

## Step 3: Separate Publication And Producer Lookup

Goal: put produced-value publication and same-block producer lookup behind
clear ownership boundaries.

Actions:

- Identify publication and producer lookup code still embedded in dispatch.
- Extract or consolidate it under concrete owner names.
- Keep dispatch responsible for invoking the owner at routing points, not for
  owning value-home mechanics.
- Avoid changing non-call lowering behavior except as required by the boundary.

Completion check:

- Publication and same-block producer responsibilities have explicit owner
  names and interfaces.
- Focused backend proof and fresh build proof pass.

## Step 4: Isolate Edge-Copy And Join-Value Handling

Goal: separate edge-copy or join-value lowering from ordinary instruction
dispatch where the current model supports it.

Actions:

- Map edge-copy and join-value call paths from dispatch.
- Move the smallest coherent edge-copy or join-value handling unit behind a
  named owner.
- Preserve the compiled machine-node route and existing diagnostics.

Completion check:

- Dispatch routes edge-copy or join-value work to a clearer owner instead of
  embedding the mechanics.
- Focused backend proof and fresh build proof pass.
- Broader backend validation is run if control-flow transfer behavior changes.

## Step 5: Remove Or Justify Call-Specific Dispatch Glue

Goal: eliminate call-specific dispatch glue only where shared call-plan facts
already own the decision.

Actions:

- Map remaining call-specific dispatch paths.
- Remove glue that duplicates available shared call-plan facts.
- Leave a concise justification for glue that still depends on unavailable
  shared facts, especially the blocked prepared call-argument source fact
  described in `ideas/open/02_aarch64_calls_emission_consolidation.md`.
- Do not create new AArch64-local call source-selection logic.

Completion check:

- Remaining call-specific dispatch glue is either gone or explicitly justified
  by a missing shared prepared fact.
- Focused call/backend proof and fresh build proof pass.

## Step 6: Boundary Documentation And Final Validation

Goal: make the new ownership boundary discoverable and prove the completed
route.

Actions:

- Add or update concise code comments where they clarify dispatch versus owner
  boundaries.
- Update existing documentation only if it already owns the relevant boundary;
  do not add markdown shards under the codegen directory.
- Run the supervisor-selected broader backend validation for the full
  dispatch responsibility reduction.

Completion check:

- `dispatch.cpp` reads as traversal/routing.
- Value materialization, publication, edge copies, producer lookup, and
  call-specific glue have explicit owners or documented justifications.
- Broader validation passes and `todo.md` records the final proof.
