# Quarantined Opaque Compatibility Memory Access Gate Runbook

Status: Active
Source Idea: ideas/open/290_gate_quarantined_opaque_compatibility_memory_accesses.md

## Purpose

Implement the next behavior-changing policy for prepared memory-access rows
whose structured provenance remains quarantined as `OpaqueCompatibility` or
`UnknownCompatible`.

## Goal

Choose and prove one target-independent policy surface that rejects, gates, or
diagnoses quarantined opaque compatibility memory accesses using explicit
opaque-compatibility metadata while preserving structured proven in-range rows.

## Core Rule

Do not gate on `UnknownCompatible` or `can_use_base_plus_offset` alone. Any
behavior-changing policy must use explicit `layout_authority ==
OpaqueCompatibility` metadata or an equivalent flattened opaque-compatibility
fact that participates in stale-row matching.

## Read First

- `ideas/open/290_gate_quarantined_opaque_compatibility_memory_accesses.md`
- `src/backend/bir/lir_to_bir/memory/memory_types.hpp`
- `src/backend/bir/lir_to_bir/memory/provenance.cpp`
- Prepared `memory_accesses` publication and lookup consumers
- Prepared publication-plan and stale-row tests covering memory accesses
- Idea 289-era structured provenance tests and fixtures

## Current Targets

- Prepared and target-independent consumers that still accept
  `OpaqueCompatibility` or `UnknownCompatible` source-memory rows.
- Lowerer-time addressed pointer load/store rows that may be rejected when
  quarantined.
- Prepared/publication consumer gates that may use `layout_authority ==
  OpaqueCompatibility` directly or a flattened equivalent fact.
- Stale-row, duplicate-row, and cross-block `memory_accesses` matching for any
  newly published opaque-compatibility fact.

## Non-Goals

- Do not replace the structured provenance carrier fields introduced by idea
  289.
- Do not treat `UnknownCompatible` alone as proof that a row came from the
  opaque pointer bridge.
- Do not add target-specific acceptance or rejection that bypasses
  target-independent source-memory provenance facts.
- Do not perform broad prepared/prealloc rewrites unrelated to quarantined
  opaque compatibility policy.
- Do not weaken supported-path expectations, stale-row matching, duplicate-row
  rejection, or cross-block rejection to make one case pass.

## Working Model

Idea 289 preserved existing opaque byte-addressed behavior by publishing
structured provenance and quarantine metadata. This runbook should first map
the remaining acceptance paths, then choose one concrete fail-closed surface.
The preferred policy surfaces are either lowerer-time rejection of quarantined
addressed pointer load/store rows or a prepared/publication consumer gate that
matches explicit opaque-compatibility facts. Structured proven in-range rows
must remain accepted throughout.

## Execution Rules

- Keep each step narrow and behavior-focused around quarantined source-memory
  rows.
- Inventory before changing behavior; do not guess which consumers still
  accept quarantined rows.
- Prefer a target-independent gate over target-specific prepared/prealloc
  handling.
- If adding a flattened opaque-compatibility fact, include it in stale-row
  matching before relying on it for acceptance policy.
- Add focused before/after tests for accepted structured rows, rejected or
  gated quarantined rows, and stale-row mismatch cases.
- Every code-changing step needs fresh build proof plus the
  supervisor-selected focused test subset.
- Broader acceptance for the behavior-changing step should include relevant
  backend/prepared validation, prepared lookup/publication-plan tests, and the
  focused structured-provenance routes affected by the selected policy.

## Step 1: Inventory Quarantined Compatibility Consumers

Goal: identify every prepared or target-independent consumer that still accepts
quarantined `OpaqueCompatibility` or `UnknownCompatible` source-memory rows and
record the viable policy surfaces.

Primary targets:

- `src/backend/bir/lir_to_bir/memory/memory_types.hpp`
- `src/backend/bir/lir_to_bir/memory/provenance.cpp`
- Prepared `memory_accesses` publication and lookup consumers
- Prepared publication-plan and stale-row tests covering memory accesses

Actions:

- Locate all published quarantine fields and all consumers that read
  `layout_authority`, `OpaqueCompatibility`, `UnknownCompatible`,
  `can_use_base_plus_offset`, or equivalent memory-access compatibility facts.
- Separate consumers that are target-independent from target-specific prepared
  or prealloc code.
- Identify addressed pointer load/store rows that can be rejected at lowering
  time without conflating structured unknown extent with opaque compatibility.
- Identify whether prepared/publication consumers can use
  `layout_authority == OpaqueCompatibility` directly or require a flattened
  opaque-compatibility fact.
- Map current stale-row, duplicate-row, and cross-block matching fields so any
  new flattened fact can be matched strictly.
- Name the focused tests that currently cover accepted structured rows,
  quarantined rows, and stale-row mismatch behavior, and list the missing tests.

Completion check:

- `todo.md` records the consumer inventory, selected candidate policy surfaces,
  matching-field implications, and focused proof subset for the next step.

## Step 2: Choose and Specify One Policy Surface

Goal: choose the first behavior-changing surface and turn it into a precise
implementation contract before editing code.

Primary targets:

- Inventory notes from `todo.md`
- The target-independent consumer or lowerer surface chosen in Step 1
- Focused tests identified in Step 1

Actions:

- Choose either lowerer-time rejection of quarantined addressed pointer
  load/store rows or a prepared/publication consumer gate.
- Define the exact predicate in terms of explicit
  `OpaqueCompatibility` metadata or an equivalent flattened fact.
- If the chosen route needs a flattened fact, specify where it is published and
  where stale-row matching must compare it.
- Define expected outcomes for structured proven in-range rows, quarantined
  rows, stale rows, duplicate rows, and cross-block rows.
- Record the narrow proof command and any broader validation the supervisor
  should require for the code-changing packet.

Completion check:

- `todo.md` contains a concrete Step 3 packet contract with the chosen surface,
  predicate, affected tests, and proof ladder.

## Step 3: Implement the Quarantine Gate

Goal: enforce the selected behavior-changing policy without weakening existing
structured-provenance or stale-row protections.

Primary targets:

- The selected lowerer or prepared/publication consumer surface
- Any carrier or publication code required for an explicit flattened
  opaque-compatibility fact
- Matching code for stale prepared `memory_accesses` rows when new facts are
  published

Actions:

- Implement the selected predicate using explicit `OpaqueCompatibility`
  metadata, not `UnknownCompatible` or `can_use_base_plus_offset` alone.
- Preserve acceptance for structured proven in-range rows.
- If publishing a flattened opaque-compatibility fact, include it in strict
  stale-row comparison before using it to gate rows.
- Keep duplicate-row and cross-block rejection at least as strict as the idea
  289 implementation.
- Avoid target-specific policy bypasses while target-independent provenance
  facts are available.

Completion check:

- The build succeeds, focused tests show structured rows accepted and
  quarantined rows rejected/gated/diagnosed at the selected surface, and stale
  or mismatched rows remain rejected.

## Step 4: Prove Behavior and Broader Prepared Safety

Goal: lock the policy with focused before/after coverage and broader
backend/prepared validation appropriate for a behavior-changing acceptance
rule.

Primary targets:

- Focused tests for the selected policy surface
- Prepared lookup/publication-plan tests
- Backend/prepared tests affected by structured source-memory provenance

Actions:

- Add or update tests for accepted structured-proof rows.
- Add or update tests for rejected, gated, or diagnosed quarantined
  `OpaqueCompatibility` rows.
- Add or update tests for stale-row mismatch involving any newly published
  opaque-compatibility fact.
- Re-run the supervisor-selected focused proof subset.
- Run broader backend/prepared validation sufficient for the changed
  acceptance policy.

Completion check:

- Focused and broader proof logs show no regression in supported
  structured-provenance routes, and the selected quarantine policy is covered
  without expectation downgrades or testcase-shaped shortcuts.
