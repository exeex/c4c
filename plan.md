# Phase F4 Memory Accesses Unsupported/Stale Proof Map Runbook

Status: Active
Source Idea: ideas/open/265_phase_f4_memory_accesses_unsupported_stale_fail_closed_proof_map.md

## Purpose

Turn the remaining `PreparedFunctionLookups::memory_accesses` unsupported and
stale fail-closed obligations into an explicit proof map after the Phase F3
Route 3 x86 `LoadLocal` agreement bridge.

## Goal

Produce a bounded analysis/proof map that names authority, compatibility
surfaces, consumer boundaries, expected fail-closed behavior, and missing
evidence for each remaining memory-access row.

## Core Rule

This runbook is analysis/proof work only. Do not demote, delete, privatize,
wrap, or migrate `PreparedFunctionLookups::memory_accesses`, and do not claim
readiness for demotion from selected supported-path evidence alone.

## Read First

- `ideas/open/265_phase_f4_memory_accesses_unsupported_stale_fail_closed_proof_map.md`
- `ideas/closed/264_phase_f4_post_f3_prepared_boundary_reassessment_gate.md`
- Relevant closure notes from Phase F3 memory/source agreement work
- Current tests, helpers, route-debug output, prepared-printer output, wrapper
  output, and target-output checks that mention `memory_accesses`, `LoadLocal`,
  source memory, prepared-only rows, stale rows, byte-offset drift, or
  cross-publication mismatch

## Current Targets

- `PreparedFunctionLookups::memory_accesses`
- Route 3 memory/source semantic authority
- Public prepared compatibility surfaces that still expose memory-access state
- x86 and riscv evidence for unsupported, stale, byte-offset drift, and
  cross-publication mismatch behavior

## Non-Goals

- Do not change implementation code as part of this activation runbook unless a
  later executor packet is explicitly delegated.
- Do not alter unsupported expectations, helper/oracle status, fallback names,
  route-debug output, prepared-printer output, wrapper output, exact target
  output, or baseline behavior to make rows look accepted.
- Do not move target-owned storage, addressing, source-home, helper, ABI,
  layout, register, stack, wrapper, formatting, emission, instruction spelling,
  or exact target-output policy into BIR.
- Do not authorize broad `PreparedFunctionLookups` retirement, prepared
  aggregate retirement, or `memory_accesses` demotion.

## Working Model

- Positive selected `LoadLocal` source-memory agreement evidence proves a
  supported path only where the same semantic authority and consumer boundary
  are exercised.
- Unsupported and stale prepared-state rows need separate fail-closed evidence.
- Public prepared compatibility may remain valid as a mirror only when the map
  names the underlying semantic authority and proves disagreement cannot be
  silently accepted.

## Execution Rules

- Keep routine evidence and packet progress in `todo.md`.
- Keep source-idea edits unnecessary unless durable intent changes.
- Treat expectation downgrades, named-fixture shortcuts, helper/status
  relabeling, or classification-only edits as route failures, not progress.
- For any code-changing packet later derived from this map, require fresh build
  or compile proof plus the supervisor-selected narrow test subset.
- Preserve canonical regression logs unless the supervisor explicitly delegates
  regeneration.

## Step 1: Gather Memory-Access Evidence

Goal: Build the factual inventory needed for the proof map.

Actions:

- Inspect the source idea and Phase F3/264 closure evidence relevant to
  `memory_accesses`.
- Locate current tests, helpers, fixtures, debug/printer checks, wrappers, and
  target-output expectations that mention memory access publication,
  `LoadLocal`, source memory, stale rows, offset drift, or cross-publication
  mismatch.
- Identify the current Route 3 semantic authority for memory/source facts.
- Identify the public prepared compatibility surfaces that must remain stable.

Completion check:

- `todo.md` records the evidence inventory, authority candidates, preserved
  compatibility surfaces, and any explicit x86/riscv non-applicability notes.

## Step 2: Map Prepared-Only Rows

Goal: Define fail-closed expectations for prepared-only memory-access rows.

Actions:

- Identify prepared-only memory rows and the consumer boundary where they are
  observed.
- For each row, name semantic authority, compatibility surface, expected
  fail-closed result, and currently available evidence.
- Separate supported-path proof from missing unsupported-row proof.

Completion check:

- `todo.md` contains a prepared-only row map that does not claim demotion
  readiness unless the required fail-closed evidence already exists.

## Step 3: Map Stale-Publication Rows

Goal: Define fail-closed expectations for stale prepared-state publication.

Actions:

- Identify stale memory-access publication rows and related helper/oracle,
  fallback, route-debug, printer, wrapper, and target-output observations.
- Name how stale prepared data should be rejected or preserved as unsupported.
- Record x86 and riscv evidence, or explicit non-applicability, for each row.

Completion check:

- `todo.md` contains a stale-publication row map with expected fail-closed
  behavior and proof gaps.

## Step 4: Map Byte-Offset Drift Rows

Goal: Define fail-closed expectations for byte-offset disagreement.

Actions:

- Identify rows where prepared memory-access byte offsets can drift from the
  Route 3 authority.
- Name the consumer boundary and observable failure/status behavior.
- Record supporting evidence, missing evidence, and preserved output surfaces.

Completion check:

- `todo.md` contains a byte-offset drift map that prevents silent acceptance of
  mismatched prepared offsets.

## Step 5: Map Cross-Publication Mismatch Rows

Goal: Define fail-closed expectations when memory-access facts disagree across
publication surfaces.

Actions:

- Identify cross-publication mismatch rows and the relevant publication
  surfaces.
- Name semantic authority, compatibility mirror behavior, and expected
  fail-closed result for each mismatch.
- Record proof gaps that must block later demotion or private-accessor work.

Completion check:

- `todo.md` contains a cross-publication mismatch map with clear blocker
  status for any unresolved row.

## Step 6: Consolidate Disposition

Goal: Turn the row maps into a final bounded disposition for the source idea.

Actions:

- Consolidate prepared-only, stale-publication, byte-offset drift, and
  cross-publication mismatch maps.
- Name which rows are supported by proof, which remain missing proof, and which
  are explicitly non-applicable.
- Decide whether any later implementation idea is safe to create, blocked, or
  out of scope.
- Do not close the source idea unless its completion criteria are satisfied and
  close-time regression guard requirements can be met.

Completion check:

- `todo.md` records the final proof map and disposition, including whether
  closure, deactivation, or follow-up idea creation should be requested.
