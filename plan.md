# AArch64 Codegen Layout Post-Prealloc Contract Audit

Status: Active
Source Idea: ideas/open/115_aarch64_codegen_layout_post_prealloc_contract_audit.md

## Purpose

Re-audit `src/backend/mir/aarch64/codegen` after the recent BIR/prealloc and
call-boundary contract work, then produce focused follow-up ideas for reducing
the AArch64 codegen surface.

## Goal

Produce a current, durable classification of oversized AArch64 codegen owners
and create only the follow-up ideas that are concrete enough for later
lifecycle activation.

## Core Rule

This plan is analysis-only. Do not edit implementation files, tests, build
metadata, or target codegen translation units while executing it.

## Read First

- `ideas/open/115_aarch64_codegen_layout_post_prealloc_contract_audit.md`
- Historical closure context for ideas 20, 34 through 39, 113, and 114 when
  needed to avoid duplicating closed work
- `src/backend/mir/aarch64/codegen`
- `ref/claudes-c-compiler/src/backend/arm/codegen`

## Current Scope

Audit and classify these AArch64 owner families:

- `calls.cpp`
- `memory.cpp`
- `alu.cpp`
- `comparison.cpp`
- `f128.cpp`
- `i128_ops.cpp`
- `machine_printer.cpp`
- `instruction.cpp`
- `dispatch.cpp`
- `dispatch_edge_copies.cpp`
- `dispatch_value_materialization.cpp`
- `dispatch_producers.cpp`

Use these classifications:

- `move-forward`: shared BIR/prealloc authority should be extracted first
- `fold-back`: target-local helper should merge into a reference-style owner
- `keep-local`: AArch64-specific emission, ABI, or representation detail is a
  justified local owner
- `contract-needed`: a high-risk area needs tests or dump visibility before
  cleanup
- `phoenix-candidate`: the file is too tangled for safe local patches and
  should be rebuilt through the phoenix markdown-review route

## Non-Goals

- Do not move, delete, merge, or rename implementation files.
- Do not change build metadata for translation units.
- Do not chase line-count parity with the reference ARM backend as the success
  metric.
- Do not reopen closed idea 20 follow-ups unless a concrete current gap remains.
- Do not start x86 or RISC-V implementation.
- Do not propose moving AArch64-specific ABI or emission policy into shared
  BIR/prealloc contracts.

## Execution Rules

- Treat closed ideas as historical context only when the active source idea
  calls for it.
- Prefer current code evidence over stale pre-113 assumptions.
- Separate shared authority migration from mechanical layout cleanup.
- Every follow-up idea must name a bounded owner, likely files, proof route,
  and concrete testcase-overfit or route-drift reject signals.
- Record "no new idea" decisions for areas already covered by closed work.
- Keep routine packet progress in `todo.md`; edit this runbook only if the
  audit route itself needs correction.

## Step 1: Establish Audit Inputs And Baseline Map

Goal: Build the current inventory and historical context needed for the audit.

Primary targets:

- `src/backend/mir/aarch64/codegen`
- `ref/claudes-c-compiler/src/backend/arm/codegen`
- relevant closed idea notes for ideas 20, 34 through 39, 113, and 114

Actions:

- Count current AArch64 and reference ARM codegen owner sizes.
- Record current largest AArch64 owner files and dispatch-family files.
- Read only the historical closure notes needed to avoid duplicate follow-up
  work.
- Identify which prior conclusions are stale because later BIR/prealloc or
  call-boundary work changed the shared/target boundary.

Completion check:

- `todo.md` contains the current owner-size inventory, reference comparison,
  and a short prior-work map that can guide classification without reopening
  closed work wholesale.

## Step 2: Classify Large Owner Families

Goal: Classify each large owner according to current code responsibility.

Primary targets:

- `calls.cpp`
- `memory.cpp`
- `alu.cpp`
- `comparison.cpp`
- `f128.cpp`
- `i128_ops.cpp`
- `machine_printer.cpp`
- `instruction.cpp`

Actions:

- Inspect each owner for target-local emission detail, representation handling,
  helper-only residue, shared semantic authority, and missing contract
  visibility.
- Assign one or more classifications from the current scope list.
- Note evidence for each classification using concrete functions, helper
  families, or data flows.

Completion check:

- `todo.md` contains a classification table for the large owners, including
  evidence and a "no new idea" note where cleanup is already covered or not
  justified.

## Step 3: Classify Dispatch-Family Residue

Goal: Map the remaining `dispatch*` files into fold-back, move-forward,
keep-local, contract-needed, or phoenix-candidate work.

Primary targets:

- `dispatch.cpp`
- `dispatch_edge_copies.cpp`
- `dispatch_value_materialization.cpp`
- `dispatch_producers.cpp`

Actions:

- Inspect dispatch entry points and helper ownership.
- Separate target-local materialization and edge-copy helpers from shared
  prepared-publication or BIR/prealloc facts.
- Identify residue that should fold into an existing owner and residue that
  should become a shared contract first.

Completion check:

- `todo.md` contains a dispatch-family residue map with concrete fold-back,
  move-forward, keep-local, and contract-needed recommendations.

## Step 4: Draft Focused Follow-Up Ideas

Goal: Create only the follow-up `ideas/open/` files justified by the audit.

Primary targets:

- `ideas/open/`
- `todo.md`

Actions:

- For each concrete next slice, create one numbered follow-up idea under
  `ideas/open/`.
- Include goal, why it exists, in-scope work, out-of-scope work, acceptance
  criteria, likely files, proof route, and reviewer reject signals.
- Prefer phoenix-style analysis for oversized tangled owners, fold-back ideas
  for target-local helper families, move-forward ideas for shared authority,
  and contract-visibility ideas where cleanup lacks reviewable proof.
- Do not create vague cleanup ideas or duplicate already closed work.

Completion check:

- Every created follow-up idea is bounded, reviewable, and traceable to the
  classification evidence in `todo.md`.

## Step 5: Prepare Closure Summary

Goal: Leave a durable closure package for the active source idea.

Primary targets:

- `todo.md`
- `ideas/open/115_aarch64_codegen_layout_post_prealloc_contract_audit.md`

Actions:

- Summarize the final owner classification table.
- Summarize the dispatch-family residue map.
- List explicit "no new idea" decisions.
- List each new follow-up idea and the evidence that justified it.
- Confirm no implementation, test, or build metadata files were changed.
- Preserve the baseline note from the source idea as the starting full-suite
  baseline:
  `5daac44dc41119b9ea3d2a8c1d91e00da78f6aec`, `<full-suite>`,
  `3427/3427 passed`.

Completion check:

- The supervisor has enough evidence to ask the plan owner to close the source
  idea if the audit outputs satisfy the acceptance criteria.
