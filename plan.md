# Phase F4 PreparedBirModule Metadata Proof Gate

Status: Active
Source Idea: ideas/open/269_phase_f4_prepared_bir_module_metadata_proof_gate.md

## Purpose

Create a proof gate for `PreparedBirModule::invariants`,
`PreparedBirModule::completed_phases`, and `PreparedBirModule::notes` before
any later private-pass-context proposal.

## Goal

Produce an evidence-backed gate that proves printer, status, debug, failure
row, direct-payload-read, absent-metadata, and retained compatibility behavior
for all three metadata fields, or records that later metadata authority work is
blocked.

## Core Rule

This runbook is proof-gate work only. Do not demote, delete, privatize, wrap,
migrate, or implement authority movement for `PreparedBirModule` metadata.

## Read First

- `ideas/open/269_phase_f4_prepared_bir_module_metadata_proof_gate.md`
- Definitions and constructors for `PreparedBirModule`
- Producers and readers of `PreparedBirModule::invariants`
- Producers and readers of `PreparedBirModule::completed_phases`
- Producers and readers of `PreparedBirModule::notes`
- Printer, status, debug, helper/oracle, fallback, wrapper, target-output, and
  unsupported paths that expose metadata-derived behavior

## Current Targets

- `PreparedBirModule::invariants`
- `PreparedBirModule::completed_phases`
- `PreparedBirModule::notes`
- Printer preservation for each metadata field
- Status and debug preservation for each metadata field
- Invalid, mismatched, missing, direct-payload-read, and absent metadata rows
- Public prepared surfaces retained as compatibility authority

## Non-Goals

- Metadata demotion, deletion, privatization, accessor wrapping, adapter
  migration, or private-pass-context implementation.
- Broad `PreparedBirModule` retirement.
- Moving target policy into BIR.
- Weakening printer, status, debug, helper/oracle, fallback, wrapper,
  target-output, unsupported behavior, or baseline expectations.
- Claiming capability progress through expectation rewrites, helper renames,
  status/oracle relabeling, printer/debug formatting changes, or
  classification-only notes.

## Working Model

- Treat all three metadata fields as retained public prepared compatibility
  authority until the proof gate shows otherwise.
- A complete gate covers each field and each failure row; proof for one field
  or one fixture is not enough.
- Direct payload reads are authority reads unless proven to be compatibility
  mirrors with preserved fail-closed behavior.
- Absent, missing, invalid, and mismatched metadata behavior must fail closed
  instead of silently accepting weaker metadata.
- If any required row lacks evidence, later private-pass-context work remains
  blocked.

## Execution Rules

- Preserve the source idea; record packet notes and evidence in `todo.md`.
- Prefer semantic metadata preservation and fail-closed proof over
  testcase-shaped matching or named fixture proof.
- For each row, name the relevant files/functions, expected fail-closed
  behavior, and compatibility surface that must stay stable.
- Do not broaden this runbook into implementation or metadata-retirement work.
- If a separate initiative appears necessary, ask the supervisor for a new
  lifecycle idea instead of expanding this runbook.

## Ordered Steps

### Step 1: Inventory Metadata Authority

Goal: identify where the three prepared metadata fields are created, stored,
published, read, printed, and observed.

Primary target: `PreparedBirModule::invariants`,
`PreparedBirModule::completed_phases`, and `PreparedBirModule::notes`.

Actions:

- Inspect `PreparedBirModule` definitions and construction paths.
- Trace producers of each metadata field.
- Trace readers of each metadata field, including direct payload reads.
- Trace printer, status, debug, helper/oracle, fallback, wrapper,
  target-output, and unsupported paths that may observe the metadata.
- List retained public prepared compatibility surfaces for each field.

Completion check:

- The execution notes name producers, readers, direct payload reads, observed
  output/status/debug paths, and retained compatibility surfaces for all three
  metadata fields without changing implementation files.

### Step 2: Map Preservation Rows

Goal: define preservation expectations for printer, status, and debug behavior
for each metadata field.

Primary target: metadata-derived printer, status, and debug surfaces.

Actions:

- Map printer preservation for `invariants`, `completed_phases`, and `notes`.
- Map status preservation for `invariants`, `completed_phases`, and `notes`.
- Map debug preservation for `invariants`, `completed_phases`, and `notes`.
- Record exact surfaces and expected behavior that must not weaken.
- Identify whether each preservation row already has supporting proof or needs
  a later code/test packet.

Completion check:

- The execution notes contain a field-by-field preservation matrix for printer,
  status, and debug behavior, with unsupported or unproven rows explicitly
  marked as blockers.

### Step 3: Map Fail-Closed Rows

Goal: cover every required failure family before later metadata authority work
can be considered.

Primary target: invalid, mismatched, missing, direct-payload-read, and absent
metadata behavior.

Actions:

- Map invalid metadata behavior for each field.
- Map mismatched metadata behavior for each field.
- Map missing metadata behavior for each field.
- Map direct-payload-read behavior for each field.
- Map absent metadata behavior for each field.
- For every row, record the expected fail-closed behavior and the evidence
  needed to prove it.

Completion check:

- The execution notes contain every required row for all three metadata fields,
  and no row is satisfied only by one named fixture or by weaker expectations.

### Step 4: Decide Gate Status

Goal: turn the preservation and fail-closed maps into a lifecycle decision.

Primary target: the completed metadata proof gate.

Actions:

- Mark each row as satisfied, blocked, or requiring a separate implementation
  idea.
- Record which public prepared surfaces remain retained compatibility
  authority.
- If all proof rows are satisfied, record the minimum conditions for a
  separate later private-pass-context proposal.
- If any proof row is missing, record that no private-pass-context, demotion,
  wrapper, adapter, or migration packet is authorized.
- Explicitly state that metadata proof does not imply whole
  `PreparedBirModule` retirement.

Completion check:

- The execution notes state whether later metadata work is blocked or eligible
  only for a separate lifecycle idea, and the decision is tied to row-map
  evidence.

### Step 5: Validate the Gate

Goal: prove the runbook result is a proof-gate lifecycle slice with no route
drift.

Primary target: planning artifacts and gathered evidence.

Actions:

- Check that no implementation files, expectations, helper/oracle statuses,
  fallback names, route-debug output, prepared-printer output, wrapper output,
  exact target output, unsupported behavior, or baselines were weakened.
- Check that the gate proves all three metadata fields instead of one field or
  one fixture.
- Check that the gate does not authorize broad `PreparedBirModule` retirement
  or metadata demotion.
- Ask the supervisor whether reviewer scrutiny is needed before closure or a
  follow-up implementation idea.

Completion check:

- The supervisor can decide whether to close, rewrite, or split the active
  lifecycle state from the proof-gate notes.
