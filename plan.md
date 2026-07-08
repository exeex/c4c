# RV64 Global Data Consumer Runbook

Status: Active
Source Idea: ideas/open/609_rv64_global_data_consumer.md
Activated from: ideas/open/609_rv64_global_data_consumer.md

## Purpose

Consume prepared global-data authority in the RV64 object route without
reconstructing facts that belong to BIR or prepared/global producers.

## Goal

Move multiple RV64/global consumer rows past the current target-side global
data stops when prepared authority already exists, while rows missing prepared
facts continue to fail with producer or authority diagnostics.

## Core Rule

RV64 may emit or load only from prepared global facts that already exist. Do
not infer global bytes, relocation slots, symbol identity, access widths, or
storage layout inside target code when upstream authority is absent.

## Read First

- ideas/open/609_rv64_global_data_consumer.md
- ideas/open/608_prepared_global_data_authority.md
- docs/rv64_gcc_torture_1000_pass_recovery/failure_bucket_map.md
- src/backend/mir/riscv/codegen/object_emission.cpp
- src/backend/mir/riscv/codegen/global_access.cpp
- src/backend/prealloc/object_data.hpp
- src/backend/prealloc/prepared_contract_verifier.cpp

## Current Targets

- RV64 global symbol emission rows that now have prepared selected
  object-data authority.
- RV64 relocation-record/object emission rows reached by relocation-only and
  mixed object-data prepared facts.
- RV64 global access-width rows whose prepared memory facts already identify a
  supported global object and access extent.

## Non-Goals

- Prepared/global authority production, including object-data, global memory
  facts, or direct global-symbol base-plus-offset publication.
- BIR initializer bootstrap, local memory, ABI, runtime/link behavior,
  expectation rewrites, unsupported-marker changes, allowlists, timeouts, or
  accounting.
- Target-side guessing of object sizes, section bytes, relocation targets, or
  access widths not published by prepared facts.

## Working Model

Ideas `608` and `620` moved representative selected object-data rows from
prepared contract stops to later RV64 consumer diagnostics. Those later stops
are the first valid ownership point for emitting prepared global objects,
relocation records, and supported accesses. This runbook owns the RV64
consumer side only: it should read prepared facts, emit supported target
artifacts, and leave missing or unsupported authority as explicit upstream
diagnostics.

## Execution Rules

- Start from fresh row inventory before editing target code.
- Keep object emission, relocation-record emission, and global access lowering
  as separate packets unless the first implementation slice proves they share
  one narrow helper.
- Preserve fail-closed diagnostics for missing prepared facts, unsupported
  sections, unknown object extents, targetless relocation slots, unsupported
  access widths, and unresolved symbol identity.
- Do not change tests, expectations, unsupported markers, or allowlists as a
  substitute for RV64 consumer behavior.
- Prove each code slice with a focused build and the supervisor-selected
  narrow RV64 gcc-torture allowlist.
- Escalate to broader validation when a slice changes shared instruction
  selection, relocation handling, or object emission helpers used outside the
  narrow global-data bucket.

## Ordered Steps

### Step 1: Inventory RV64 global consumer stops

Goal: classify current RV64/global rows by first consumer stop after prepared
authority is available.

Primary targets:
- `build/rv64_gcc_c_torture_backend/<case-id>/case.log`
- prepared object-data and global-access diagnostics
- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `src/backend/mir/riscv/codegen/global_access.cpp`

Actions:
- Build an allowlist with relocation-only, mixed object-data, symbol-emission,
  and access-width representatives where prepared facts already exist.
- Record each row's first current RV64 diagnostic and the prepared facts it
  depends on.
- Separate rows still missing prepared/global authority from rows owned by
  RV64 consumers.
- Choose the first implementation packet from the largest confirmed consumer
  family.

Completion check:
- `todo.md` records the selected allowlist, row classification, first owned
  RV64 consumer family, and exact proof command for Step 2.

### Step 2: Consume prepared object-data symbol publication

Goal: emit supported prepared global object symbols and sections when object
data facts are coherent.

Primary targets:
- `src/backend/mir/riscv/codegen/object_emission.cpp`
- RV64 object emission tests for prepared global object data

Actions:
- Emit object labels and section placement from prepared object-data records
  that have publication identity, byte range, size, and alignment.
- Preserve explicit rejection for missing labels, missing publication identity,
  unsupported section kinds, unknown byte ranges, and unresolved link names.
- Keep relocation-record and access-width handling out of this packet unless
  the selected rows require only a minimal handoff to keep object emission
  coherent.

Completion check:
- Multiple symbol-emission rows with prepared object-data facts move past the
  prior RV64 symbol/object publication stop, or the packet returns evidence
  that relocation-record emission is the true first consumer blocker.

### Step 3: Emit prepared relocation records for object data

Goal: consume prepared relocation slots without reconstructing relocation
targets or offsets in RV64 code.

Primary targets:
- `src/backend/mir/riscv/codegen/object_emission.cpp`
- object emission tests covering relocation-only and mixed object-data records

Actions:
- Translate prepared relocation slots into RV64 object relocation records only
  when offset, size, target identity, and object extent are known.
- Preserve fail-closed diagnostics for targetless, out-of-range, overlapping,
  unsupported-width, or unsupported-section relocation slots.
- Keep ordinary emitted bytes sourced from prepared object-data records.

Completion check:
- Relocation-only and mixed object-data rows that already reached RV64
  relocation-record diagnostics progress to the next downstream owner or pass
  object emission, with no target-local inference of missing prepared facts.

### Step 4: Lower supported global access widths

Goal: handle RV64 loads and stores for prepared global accesses whose width
and object identity are already authoritative.

Primary targets:
- `src/backend/mir/riscv/codegen/global_access.cpp`
- RV64 instruction selection tests for global load/store widths

Actions:
- Lower supported byte, halfword, word, doubleword, and pointer-sized global
  accesses only when prepared facts identify the global object and access
  extent.
- Preserve diagnostics for missing prepared memory facts, unsupported widths,
  volatile/atomic gaps, and ambiguous base-plus-offset authority.
- Do not move prepared memory fact production into target lowering.

Completion check:
- Multiple access-width rows progress when prepared authority exists, while
  rows still missing prepared facts remain assigned to producer authority.

### Step 5: Prove consumer handoff and residual ownership

Goal: validate the RV64/global consumer boundary and record any remaining
producer or downstream owner split.

Primary targets:
- supervisor-selected RV64 global-data allowlist
- `todo.md`
- `ideas/open/608_prepared_global_data_authority.md` only if the supervisor
  delegates lifecycle closure or route repair

Actions:
- Re-run the focused build and narrow allowlist after implementation.
- Record rows that moved through RV64 object emission, relocation records, or
  global access-width handling.
- Record rows that still belong to prepared/global authority, runtime/link,
  ABI, or another downstream owner.
- Return to the supervisor for review, broader validation, or lifecycle
  routing.

Completion check:
- `todo.md` contains the movement summary, residual owner split, proof
  command, and whether the source idea is ready for close review.
