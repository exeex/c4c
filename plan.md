# BIR Prealloc Control Publication Lookup Boundary Audit Runbook

Status: Active
Source Idea: ideas/open/99_bir_prealloc_control_publication_lookup_boundary_audit.md

## Purpose

Audit whether prealloc duplicates BIR control-flow and publication semantics, then produce follow-up ideas only for concrete authority overlaps or contract-test gaps.

## Goal

Classify the BIR/prealloc control, publication, and prepared-lookup boundary before x86/RISC-V rebuild work starts.

## Core Rule

This is analysis-only work. Do not edit implementation files or create arch-specific implementation tasks unless the audit traces a concrete shared contract gap first.

## Read First

- `ideas/open/99_bir_prealloc_control_publication_lookup_boundary_audit.md`
- `src/backend/bir/bir.hpp`
- `src/backend/bir/lir_to_bir/cfg.cpp`
- `src/backend/bir/lir_to_bir/scalar.cpp`
- `src/backend/bir/lir_to_bir/module.cpp`
- `src/backend/prealloc/out_of_ssa.cpp`
- `src/backend/prealloc/control_flow.hpp`
- `src/backend/prealloc/publication_plans.*`
- `src/backend/prealloc/prepared_lookups.*`
- `src/backend/prealloc/formal_publications.*`
- `src/backend/prealloc/prepared_printer/*`

## Current Targets

- BIR CFG, terminators, phi/select/comparison semantics, and instruction values.
- Prealloc out-of-SSA join transfers.
- Prepared edge publications and current-block entry publications.
- Select-chain and compare-join classification.
- Store/source publication plans.
- Prepared lookup APIs used by arch consumers.
- Prepared printer and dump contract coverage.

## Non-Goals

- Do not implement control-flow or publication cleanup.
- Do not move register/storage placement or target emission details into BIR.
- Do not reopen AArch64 dispatch/publication cleanup routes.
- Do not create x86/RISC-V arch implementation work before shared contract gaps are named.
- Do not rewrite prepared lookup APIs without tracing consumer-facing gaps.

## Working Model

Classify each overlap with one of these labels:

- `bir-control-semantic-fact`
- `prealloc-transfer-plan-authority`
- `prealloc-rederives-bir-control-fact`
- `bir-missing-target-neutral-fact`
- `lookup-api-contract-gap`
- `prepared-printer-contract-gap`
- `needs-follow-up-idea`

Use `todo.md` to record running inventories, classifications, proof notes, and any generated follow-up idea paths.

## Execution Rules

- Prefer source reads, symbol tracing, and dump/contract inspection over code changes.
- If a distinct implementation initiative is discovered, create a focused follow-up idea under `ideas/open/` with concrete reviewer reject signals.
- Keep routine audit notes in `todo.md`; edit this plan only if the route itself needs repair.
- Do not edit the source idea unless activation/closure metadata or durable intent changes are required.
- Analysis-only close does not require backend tests unless implementation files are accidentally changed; close must still report guard status.

## Ordered Steps

### Step 1: Inventory BIR Control Facts

Goal: identify the target-neutral facts BIR already owns.

Primary targets:

- `src/backend/bir/bir.hpp`
- `src/backend/bir/lir_to_bir/cfg.cpp`
- `src/backend/bir/lir_to_bir/scalar.cpp`
- `src/backend/bir/lir_to_bir/module.cpp`

Actions:

- Inspect BIR CFG, terminator, phi, select, compare, and instruction-value carriers.
- Record which facts are explicit BIR semantic authority.
- Note any missing target-neutral fact that prealloc appears forced to reconstruct later.

Completion check:

- `todo.md` contains an inventory of BIR control facts and any provisional `bir-missing-target-neutral-fact` entries.

### Step 2: Inventory Prealloc Transfer And Publication Plans

Goal: identify which prealloc facts are legitimate transfer/publication plans versus duplicated control semantics.

Primary targets:

- `src/backend/prealloc/out_of_ssa.cpp`
- `src/backend/prealloc/control_flow.hpp`
- `src/backend/prealloc/publication_plans.*`
- `src/backend/prealloc/formal_publications.*`

Actions:

- Trace out-of-SSA join transfers.
- Trace edge publications and current-block entry publications.
- Trace store/source publication plans.
- Classify each fact as `prealloc-transfer-plan-authority`, `prealloc-rederives-bir-control-fact`, or another listed label.

Completion check:

- `todo.md` contains a classification table for transfer and publication facts, with file/function evidence for each overlap.

### Step 3: Inventory Select And Compare Join Classification

Goal: determine whether prealloc classification re-derives BIR select/compare semantics or only prepares target-facing plans.

Primary targets:

- `src/backend/prealloc/out_of_ssa.cpp`
- `src/backend/prealloc/control_flow.hpp`
- `src/backend/prealloc/publication_plans.*`
- `src/backend/prealloc/prepared_lookups.*`

Actions:

- Trace select-chain and compare-join classification.
- Compare the classification inputs to BIR select, compare, and terminator facts.
- Name any duplicated semantic authority or missing BIR/prepared contract.

Completion check:

- `todo.md` records the select/compare classification boundary and marks each issue as retained, follow-up-worthy, or no-overlap.

### Step 4: Audit Prepared Lookup APIs And Prepared Printer Coverage

Goal: identify consumer-facing lookup or dump contract gaps before arch rebuild work depends on them.

Primary targets:

- `src/backend/prealloc/prepared_lookups.*`
- `src/backend/prealloc/prepared_printer/*`
- arch consumers reached from prepared lookup APIs, if needed for evidence.

Actions:

- Inventory prepared lookup APIs used by arch consumers.
- Check whether lookup names expose stable contract facts or force consumers to infer semantics from prepared shapes.
- Check prepared printer/dump coverage for the audited facts.
- Classify gaps as `lookup-api-contract-gap` or `prepared-printer-contract-gap`.

Completion check:

- `todo.md` lists lookup and printer gaps that should become tests or follow-up ideas before x86/RISC-V rebuild work starts.

### Step 5: Produce Follow-Up Ideas And Close Readiness Summary

Goal: convert only traced overlaps or contract gaps into durable follow-up ideas.

Primary targets:

- `ideas/open/`
- `todo.md`

Actions:

- Create focused follow-up ideas only for concrete traced overlaps or contract-test gaps.
- Include concrete `## Reviewer Reject Signals` in every new idea.
- Summarize all classifications, generated follow-up idea paths, and no-action findings in `todo.md`.
- Prepare the close decision for supervisor/plan-owner review.

Completion check:

- Every `needs-follow-up-idea` item has a corresponding idea path under `ideas/open/`, and `todo.md` is ready for source-idea close evaluation.
