# BIR/prealloc Prepared Query Surface Simplification Audit

Status: Active
Source Idea: ideas/open/136_bir_prealloc_prepared_query_surface_simplification_audit.md

## Purpose

Audit the BIR/prealloc prepared-query and contract surfaces after the AArch64
cleanup series, especially ideas 130-135, and decide whether the shared layer
should stay as-is, move query ownership by domain, split large facades, or spawn
bounded follow-up simplification ideas.

## Goal

Produce an analysis-only closure note with a prepared-query table, ownership
map, consumer map, no-new-idea notes where appropriate, and concrete follow-up
ideas only when the audit finds real simplification opportunities.

## Core Rule

Do not directly rewrite prealloc, BIR, AArch64 codegen, tests, or expectations
while executing this plan. This runbook is for analysis and follow-up idea
creation only.

## Read First

- `ideas/open/136_bir_prealloc_prepared_query_surface_simplification_audit.md`
- closure records or archived notes for ideas 130-135
- `src/backend/prealloc/prepared_lookups.cpp`
- `src/backend/prealloc/prepared_lookups.hpp`
- `src/backend/prealloc/call_plans.cpp`
- `src/backend/prealloc/publication_plans.cpp`
- `src/backend/prealloc/control_flow.hpp`
- `src/backend/prealloc/out_of_ssa.cpp`
- adjacent BIR/prealloc headers and AArch64 backend consumers discovered by
  symbol search

## Current Targets / Scope

Classify prepared-query and contract surfaces in these domains:

- value-home and prepared storage lookup
- current-block entry publication
- edge-publication and move-bundle facts
- same-block producer and materialization facts
- select-chain dependency facts
- call-plan argument, result, and outgoing-stack facts
- memory-access and pointer/value-home facts
- preservation and runtime-helper facts

For each domain, identify owning files, public query APIs, AArch64 consumers,
target-neutral semantics, duplicate or overly narrow query shapes, natural
owner placement, and likely value for future x86/riscv work.

## Non-Goals

- Do not perform implementation changes.
- Do not reduce line count mechanically without an ownership argument.
- Do not move target-local AArch64 register spelling, hazard policy, scratch
  choices, or final instruction emission into shared code.
- Do not reopen closed AArch64 dispatch, calls, memory, or wide-value fixes
  unless a current shared-query smell proves they left duplicated policy behind.
- Do not redesign x86 or riscv backends before the shared contract shape is
  clear.

## Working Model

The audit should separate good shared semantic facts from AArch64-shaped helper
facades. A large shared file is acceptable when it expresses reusable backend
contracts; a smaller file is not progress if it pushes AArch64 consumers back to
local BIR scans, name matching, predecessor rescans, or testcase-shaped logic.

## Execution Rules

- Use semantic ownership as the organizing principle, not line counts.
- Compare new or recent shared queries against the closures of ideas 130-135.
- Prefer `rg` and AST-backed tools where useful for query and consumer mapping.
- Record explicit no-new-idea decisions for clean query groups.
- When proposing follow-up ideas, make each one bounded by files, proof route,
  and reviewer reject signals.
- Keep routine execution notes in `todo.md`; edit this plan only if the audit
  route itself needs repair.

## Ordered Steps

### Step 1: Establish Audit Inventory

Goal: collect the exact query surfaces and historical context the audit will
classify.

Primary targets:

- closure records or archived notes for ideas 130-135
- `src/backend/prealloc/prepared_lookups.*`
- `src/backend/prealloc/call_plans.cpp`
- `src/backend/prealloc/publication_plans.cpp`
- `src/backend/prealloc/control_flow.hpp`
- `src/backend/prealloc/out_of_ssa.cpp`

Actions:

- inspect ideas 130-135 closure records and list the shared queries they
  introduced or justified
- enumerate public prepared/prealloc query APIs and adjacent helper groups
- group the APIs by the domains listed in this plan
- note missing context or uncertain provenance in `todo.md`

Completion check:

- `todo.md` contains a domain-grouped inventory of query groups and the
  relevant 130-135 closure context needed for classification.

### Step 2: Map Owners and AArch64 Consumers

Goal: prove where each query group is owned and who still depends on it.

Primary targets:

- BIR/prealloc owner files found in Step 1
- AArch64 backend files discovered by symbol or text search

Actions:

- map each query group to the file that owns its data and public API
- identify every AArch64 consumer for each query group
- mark whether each consumer uses a semantic prepared fact or an AArch64-shaped
  helper surface
- identify duplicated names, split-too-narrow query pairs, and owner-file
  mismatches

Completion check:

- `todo.md` contains a consumer map and ownership map detailed enough to support
  accept/reject decisions for each major query group.

### Step 3: Classify Shared Contract Quality

Goal: decide which query groups are clean shared semantics, misplaced facts,
  overly narrow surfaces, suspicious facades, or obsolete after later cleanup.

Actions:

- classify each domain with one of:
  - good shared semantic fact
  - good fact in the wrong owner file
  - too narrow or duplicated
  - suspicious AArch64-shaped facade
  - no longer needed after later cleanup
- explain why simplification would help future x86/riscv work or why it would
  only reshuffle AArch64 code
- record explicit no-new-idea notes for groups that are already coherent

Completion check:

- `todo.md` records classification decisions with enough evidence to discourage
  broad rewrites and testcase-shaped follow-ups.

### Step 4: Draft Bounded Follow-Up Ideas If Needed

Goal: create durable follow-up idea drafts only for concrete simplification
  slices justified by the audit.

Actions:

- for each real opportunity, define bounded files and query groups
- include an implementation proof route, likely narrow tests, and broader check
  conditions
- include concrete reviewer reject signals, especially for target-local policy
  leakage, AArch64 rediscovery, expectation downgrades, and vague cleanup
- avoid creating follow-up ideas for query groups that are already clean or
  whose simplification would only move code around

Completion check:

- any proposed follow-up ideas are concrete enough to be activated separately,
  and clean groups have explicit no-new-idea notes.

### Step 5: Assemble Closure Recommendation

Goal: prepare the final analysis outcome for lifecycle closure.

Actions:

- assemble the major prepared-query table
- assemble the domain ownership map
- assemble the AArch64 consumer map
- summarize no-new-idea decisions
- summarize follow-up idea recommendations, if any
- recommend whether `prepared_lookups.*` should stay as one facade, split by
  domain, or shrink by moving queries to existing owners

Completion check:

- `todo.md` contains the complete analysis package needed for plan-owner close
  review, with no implementation files changed by this analysis plan.
