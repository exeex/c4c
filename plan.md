# Phase F4 Edge Publications Unsupported Publication Proof Map Runbook

Status: Active
Source Idea: ideas/open/266_phase_f4_edge_publications_unsupported_publication_fail_closed_proof_map.md

## Purpose

Turn the remaining `PreparedFunctionLookups::edge_publications` unsupported
publication obligations into an explicit fail-closed proof map after the Phase
F3 x86 Route 5 prepared edge-publication agreement bridge.

## Goal

Produce a bounded analysis/proof map that names publication authority,
consumer boundaries, compatibility surfaces, expected fail-closed behavior, and
missing evidence for each remaining unsupported edge-publication row.

## Core Rule

This runbook is analysis/proof work only. Do not demote, delete, privatize,
wrap, or migrate `PreparedFunctionLookups::edge_publications`, and do not
claim readiness for demotion from selected supported-path evidence alone.

## Read First

- `ideas/open/266_phase_f4_edge_publications_unsupported_publication_fail_closed_proof_map.md`
- `ideas/closed/264_phase_f4_post_f3_prepared_boundary_reassessment_gate.md`
- Relevant Phase F3 closure notes for x86 Route 5 prepared edge-publication
  agreement and Route 3 agreement gating
- Current tests, helpers, route-debug output, prepared-printer output, wrapper
  output, and target-output checks that mention `edge_publications`, Route 5
  records, natural edges, prepared-only rows, Route 5-only rows, or wrong-edge
  publication rows

## Current Targets

- `PreparedFunctionLookups::edge_publications`
- Route 5/BIR publication authority for natural-edge publication records
- Public prepared compatibility surfaces that still expose edge-publication
  state
- x86 and riscv evidence for duplicate same-edge, prepared-only, Route 5-only,
  and wrong-edge unsupported publication behavior

## Non-Goals

- Do not change implementation code as part of this activation runbook unless a
  later executor packet is explicitly delegated.
- Do not alter unsupported expectations, helper/oracle status, fallback names,
  route-debug output, prepared-printer output, wrapper output, exact target
  output, or baseline behavior to make rows look accepted.
- Do not move target-owned edge layout, branch/layout, publication, output,
  formatting, emission, instruction spelling, wrapper, ABI, register, stack,
  storage, or exact target-output policy into BIR.
- Do not authorize broad `PreparedFunctionLookups` retirement, prepared
  aggregate retirement, or `edge_publications` demotion.

## Working Model

- Positive selected x86 dynamic `LoadLocal` Route 5/prepared
  edge-publication agreement proves a supported path only where the same
  semantic authority and consumer boundary are exercised.
- Unsupported publication rows need separate fail-closed evidence for every
  still-public prepared lookup, helper/oracle/status/fallback/debug/printer,
  wrapper-output, target-output, and baseline surface they can affect.
- Public prepared compatibility may remain valid as a mirror only when the map
  names the underlying Route 5/BIR publication authority and proves
  disagreement cannot be silently accepted.

## Execution Rules

- Keep routine evidence and packet progress in `todo.md`.
- Keep source-idea edits unnecessary unless durable intent changes.
- Treat expectation downgrades, named-fixture shortcuts, helper/status
  relabeling, route-debug/printer output changes, or classification-only edits
  as route failures, not progress.
- For any code-changing packet later derived from this map, require fresh build
  or compile proof plus the supervisor-selected narrow test subset.
- Preserve canonical regression logs unless the supervisor explicitly
  delegates regeneration.

## Step 1: Gather Edge-Publication Evidence

Goal: Build the factual inventory needed for the proof map.

Actions:

- Inspect the source idea and Phase F3/264 closure evidence relevant to
  `edge_publications`.
- Locate current tests, helpers, fixtures, debug/printer checks, wrappers, and
  target-output expectations that mention edge publications, Route 5 records,
  natural edges, prepared-only publication rows, Route 5-only rows, or
  wrong-edge rows.
- Identify the current Route 5/BIR semantic authority for edge-publication
  facts.
- Identify the public prepared compatibility surfaces that must remain stable.

Completion check:

- `todo.md` records the evidence inventory, authority candidates, preserved
  compatibility surfaces, and any explicit x86/riscv non-applicability notes.

## Step 2: Map Duplicate Same-Edge Route 5 Records

Goal: Define fail-closed expectations for duplicate Route 5 publication records
on one natural edge.

Actions:

- Identify duplicate same-edge Route 5 rows and the consumer boundary where
  duplicate publication can be observed.
- For each row, name semantic authority, compatibility surface, expected
  fail-closed result, and currently available evidence.
- Separate selected supported-path proof from missing unsupported-row proof.

Completion check:

- `todo.md` contains a duplicate same-edge Route 5 row map that does not claim
  demotion readiness unless the required fail-closed evidence already exists.

## Step 3: Map Prepared-Only Publication Rows

Goal: Define fail-closed expectations for prepared-only edge-publication rows.

Actions:

- Identify prepared-only publication rows and any helper/oracle, fallback,
  route-debug, printer, wrapper, and target-output observations.
- Name how prepared-only publication data should be rejected or preserved as
  unsupported compatibility state.
- Record x86 and riscv evidence, or explicit non-applicability, for each row.

Completion check:

- `todo.md` contains a prepared-only publication row map with expected
  fail-closed behavior and proof gaps.

## Step 4: Map Route 5-Only Publication Rows

Goal: Define fail-closed expectations when Route 5 publication exists without
matching prepared compatibility publication.

Actions:

- Identify Route 5-only publication rows and the consumer boundary where
  missing prepared compatibility data can matter.
- Name the Route 5/BIR authority, retained compatibility mirror behavior, and
  observable failure/status behavior for each row.
- Record supporting evidence, missing evidence, and preserved output surfaces.

Completion check:

- `todo.md` contains a Route 5-only publication map that distinguishes real
  Route 5 authority from prepared compatibility gaps.

## Step 5: Map Wrong-Edge Publication Rows

Goal: Define fail-closed expectations for publication rows attached to the
wrong natural edge.

Actions:

- Identify wrong-edge publication rows and the relevant natural-edge,
  predecessor/successor, Route 5, and prepared publication surfaces.
- Name semantic authority, compatibility mirror behavior, and expected
  fail-closed result for each mismatch.
- Record proof gaps that must block later demotion or private-accessor work.

Completion check:

- `todo.md` contains a wrong-edge publication map with clear blocker status for
  any unresolved row.

## Step 6: Consolidate Disposition

Goal: Turn the row maps into a final bounded disposition for the source idea.

Actions:

- Consolidate duplicate same-edge Route 5, prepared-only, Route 5-only, and
  wrong-edge publication maps.
- Name which rows are supported by proof, which remain missing proof, and which
  are explicitly non-applicable.
- Decide whether any later implementation idea is safe to create, blocked, or
  out of scope.
- Do not close the source idea unless its completion criteria are satisfied and
  close-time regression guard requirements can be met.

Completion check:

- `todo.md` records the final proof map and disposition, including whether
  closure, deactivation, or follow-up idea creation should be requested.
