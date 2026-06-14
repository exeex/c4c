# Phase F3 Prepared Private Pass-Context Metadata Gate

Status: Active
Source Idea: ideas/open/255_phase_f3_prepared_private_pass_context_metadata_gate.md

## Purpose

Gate a narrow private-pass-context demotion decision for four
`PreparedBirModule` metadata fields:

- `route`
- `invariants`
- `completed_phases`
- `notes`

## Goal

Determine, field by field, whether direct public access can be replaced with a
small private pass-context accessor or adapter while preserving all current
printer, status, debug, and fallback behavior.

## Core Rule

Do not demote, delete, rename, or hide any field unless every current consumer
is accounted for and compatibility output remains exact. If a target-facing
consumer still needs direct public access, mark that field blocked.

## Read First

- Source idea:
  `ideas/open/255_phase_f3_prepared_private_pass_context_metadata_gate.md`
- Current `PreparedBirModule` field declarations and construction sites.
- Prepared printer, status, debug, and fallback tests that mention route,
  invariants, completed phases, notes, or absent-note behavior.
- x86 and riscv prepared consumers that read `PreparedBirModule` directly.

## Current Scope

In scope:

- Inventory all current consumers for the four named fields.
- Preserve prepared printer route output, invariant output, completed-phase
  text/status rows, notes text/status rows, absent-note fallback behavior, and
  debug/status strings.
- Define the smallest accessor or adapter shape needed for any accepted
  demotion.
- Prove that x86/riscv target-facing public consumers do not require direct
  field access, or mark the field blocked.

Non-goals:

- Delete any of the four fields.
- Touch `PreparedBirModule::liveness`.
- Reclassify metadata as BIR semantic fact or target policy.
- Rewrite prepared printer/status/debug text or weaken diagnostics.
- Retire the broader `PreparedBirModule` aggregate.

## Working Model

- These fields are metadata/pass-context candidates only.
- Compatibility output remains owned by the prepared printer/status/debug
  surfaces during this runbook.
- A field can end in one of three states:
  accepted for private-pass-context demotion, retained as public
  compatibility, or blocked with a named consumer.

## Execution Rules

- Keep changes field-scoped and behavior-preserving.
- Prefer consumer inventory and compatibility proof before any access change.
- Do not treat helper renames, accessor renames, or classification notes as
  demotion progress.
- Do not extend existing special-case code or add named-test shortcuts.
- For code-changing steps, run a fresh build or compile proof plus focused
  tests covering the touched compatibility surface.
- Escalate to broader validation if more than one field or shared printer/debug
  behavior changes in one slice.

## Steps

### Step 1: Consumer Inventory

Goal: identify every direct and indirect consumer of the four fields.

Primary targets:

- `PreparedBirModule::route`
- `PreparedBirModule::invariants`
- `PreparedBirModule::completed_phases`
- `PreparedBirModule::notes`

Actions:

- Search for each field's direct reads, writes, and aggregate construction.
- Classify consumers as printer/status/debug compatibility, pass-context
  metadata, target-facing backend use, tests, or construction-only.
- Identify any x86/riscv public consumer that would block demotion.
- Record unresolved consumers in `todo.md` rather than changing source intent.

Completion check:

- Each field has a named consumer list and preliminary state:
  candidate, public compatibility retained, or blocked with reason.

### Step 2: Compatibility Contract Map

Goal: make the observable behavior that must not change explicit before code
changes.

Actions:

- Locate tests or expectations for route output, invariant output,
  completed-phase text/status rows, notes text/status rows, absent-note
  fallback, and debug/status strings.
- Identify missing fail-closed coverage for invalid, mismatched, absent, or
  fallback state relevant to these fields.
- Choose the smallest focused proof subset for the first implementation slice.

Completion check:

- `todo.md` names the compatibility rows and proof command the executor should
  preserve for the first selected field or field group.

### Step 3: Accessor Or Adapter Shape

Goal: define the smallest private pass-context access shape for accepted
candidate fields.

Actions:

- For each accepted candidate, introduce or identify a private accessor,
  adapter, or pass-context query that preserves the existing compatibility
  surface.
- Keep retained public compatibility explicit when direct field exposure cannot
  be removed safely.
- Keep fail-closed handling for invalid or mismatched state.

Completion check:

- The chosen shape removes or narrows semantic reliance on public field access
  for the selected candidate without changing output contracts.

### Step 4: Field-Scoped Demotion Slice

Goal: apply one narrow demotion or retained-compatibility decision at a time.

Actions:

- Change only the selected field or tightly coupled field group.
- Preserve printer/status/debug text byte-for-byte unless the supervisor
  explicitly approves a contract change.
- Keep target-facing direct public consumers intact if they were marked
  blocked.
- Run build or compile proof plus the focused compatibility subset.

Completion check:

- The selected field is either demoted behind the accepted pass-context shape,
  retained as public compatibility with reason, or blocked with a named
  consumer and proof notes.

### Step 5: Final Field Decision Matrix

Goal: finish the gate with durable execution-state evidence for all four
fields.

Actions:

- Ensure `todo.md` records the final state of `route`, `invariants`,
  `completed_phases`, and `notes`.
- Confirm `liveness` stayed outside the runbook.
- Run the supervisor-selected broader validation checkpoint if any code slices
  changed shared printer/status/debug behavior.

Completion check:

- All four fields have accepted, retained, or blocked outcomes with proof
  notes, and no source idea change is needed unless the supervisor chooses to
  close or split the idea.
