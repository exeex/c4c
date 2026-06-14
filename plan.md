# Phase F3 Prepared Module Structural Exit Blocker Map

Status: Active
Source Idea: ideas/open/253_phase_f3_prepared_module_structural_exit_blocker_map.md

## Purpose

Turn the prepared module structural field exit question into a concrete blocker
map for future one-field or one-reader packets.

## Goal

Map safe exit prerequisites for `PreparedBirModule::module`,
`PreparedBirModule::names`, `PreparedBirModule::control_flow`, and
`PreparedBirModule::store_source_publications` without proposing aggregate
retirement or field deletion.

## Core Rule

Do not claim structural exit readiness unless the semantic fact, consumer,
compatibility surface, and fail-closed proof set are all concrete.

## Read First

- `ideas/open/253_phase_f3_prepared_module_structural_exit_blocker_map.md`
- Existing prepared module, lookup, printer, route-debug, fallback, and target
  output consumers before recording any row as implementation-ready.

## Current Targets And Scope

- `PreparedBirModule::module`
- `PreparedBirModule::names`
- `PreparedBirModule::control_flow`
- `PreparedBirModule::store_source_publications`
- Route/BIR semantic facts, retained compatibility surfaces, target-policy
  surfaces, and blocked prepared authority for each field.

## Non-Goals

- Do not delete, privatize, or wrap any of the four fields.
- Do not open broad `PreparedBirModule` retirement or draft 155 work.
- Do not move target policy into BIR.
- Do not rewrite printer, debug, route-debug, helper/oracle, fallback, or
  output strings.
- Do not edit implementation code as part of this analysis-only runbook unless
  the supervisor explicitly switches this idea into an implementation packet.

## Working Model

- Treat `ideas/open/253_phase_f3_prepared_module_structural_exit_blocker_map.md`
  as durable source intent.
- Record routine analysis progress and proof notes in `todo.md`.
- Update this runbook only if the blocker-map route needs a true lifecycle
  correction.
- Leave the source idea unchanged unless source intent changes.

## Execution Rules

- Prefer consumer-backed rows over speculative categories.
- Keep rows blocked when implementation shape or proof scope is not concrete.
- Distinguish BIR semantic facts from compatibility formatting, target policy,
  and public prepared aggregate authority.
- Treat testcase-shaped or string-only evidence as insufficient.
- Preserve public prepared aggregate compatibility until migrated consumers or
  explicit retained adapters are identified.

## Ordered Steps

### Step 1: Inventory Field Consumers And Authority Surfaces

Goal: Build the consumer inventory for all four target fields.

Primary target: prepared module readers and downstream prepared lookup,
printer, route-debug, fallback, and target-output consumers.

Actions:

- Inspect readers of `PreparedBirModule::module`, `names`, `control_flow`, and
  `store_source_publications`.
- Classify each reader as route/BIR semantic fact, retained compatibility
  surface, target-policy surface, or blocked prepared authority.
- Note unresolved consumers in `todo.md` instead of promoting speculation into
  the source idea.

Completion check:

- Every target field has a current consumer list with at least one initial
  classification per reader or an explicit blocked/unknown note.

### Step 2: Map `module` Structural Exit Preconditions

Goal: Separate BIR module/function/block structure from prepared aggregate
handoff and printer/module output.

Primary target: `PreparedBirModule::module`.

Actions:

- Identify which `module` consumers need semantic module, function, or block
  structure.
- Identify compatibility consumers that still require prepared aggregate
  shape, printer text, or module output.
- Record any future one-reader packet only if the reader, semantic fact,
  compatibility surface, and fail-closed proof are concrete.

Completion check:

- The `module` row names concrete future packets where possible and leaves
  non-concrete exits blocked.

### Step 3: Map `names` Structural Exit Preconditions

Goal: Separate semantic name-to-id lookup from debug text, route-debug names,
printer formatting, target formatting, fallback rendering, and conflict
failures.

Primary target: `PreparedBirModule::names`.

Actions:

- Identify direct semantic lookup readers and non-semantic formatting readers.
- Preserve duplicate/conflict name failure behavior as a compatibility surface.
- Mark AArch64 formatting, fallback rendering, route-debug names, and printer
  output as retained compatibility unless a concrete adapter shape exists.

Completion check:

- The `names` row describes semantic lookup candidates separately from every
  retained name-formatting and fail-closed compatibility surface.

### Step 4: Map `control_flow` Structural Exit Preconditions

Goal: Separate CFG, dominance, and block-index facts from branch lowering,
layout, labels, and instruction spelling.

Primary target: `PreparedBirModule::control_flow`.

Actions:

- Identify CFG/dominance/block-index readers that could be semantic facts.
- Identify branch lowering, layout, label, and spelling consumers that remain
  compatibility or target-policy owned.
- Keep broad control-flow lowering or layout rewrites out of scope.

Completion check:

- The `control_flow` row names any bounded semantic-reader candidate and keeps
  target-policy or output-sensitive rows blocked.

### Step 5: Map `store_source_publications` Structural Exit Preconditions

Goal: Separate source/publication semantic records from source-memory/status
compatibility and addressing/storage output.

Primary target: `PreparedBirModule::store_source_publications`.

Actions:

- Identify consumers of source/publication semantic records.
- Identify source-memory status, publication status, addressing, storage, and
  output-format consumers.
- Require concrete fail-closed proof for missing, invalid, mismatch, fallback,
  and policy-sensitive rows before naming implementation packets.

Completion check:

- The `store_source_publications` row distinguishes semantic records from
  retained compatibility and target-output surfaces.

### Step 6: Normalize The Blocker Map And Proof Needs

Goal: Produce an executor/reviewer usable summary of what can be implemented
later and what remains blocked.

Primary target: `todo.md` progress notes for this active analysis runbook.

Actions:

- Consolidate per-field classifications into a compact blocker map.
- For each future packet candidate, name the semantic fact, consumer,
  compatibility surface, and fail-closed proof set.
- For each blocked row, record the missing fact, consumer, adapter, or proof.

Completion check:

- A future supervisor can choose one-field or one-reader packets without
  re-deriving the structural exit map.
