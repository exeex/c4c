# Prepared Mixed Object Data Slots Runbook

Status: Active
Source Idea: ideas/open/620_prepared_mixed_object_data_slots.md
Activated from: ideas/open/620_prepared_mixed_object_data_slots.md

## Purpose

Split the mixed object-data representation gap out of `608` and make prepared
facts capable of carrying ordinary bytes plus relocation slots before RV64
global emission consumes them.

## Goal

Move at least one mixed selected object-data row past the prepared contract
stop for a semantic prepared-fact reason, while preserving RV64 relocation and
object-emission work for `609`.

## Core Rule

Represent mixed object data in prepared facts first. Do not repair this idea by
emitting RV64 relocation records, reconstructing bytes in target code, changing
expectations, or matching representative testcase names.

## Read First

- ideas/open/620_prepared_mixed_object_data_slots.md
- ideas/open/608_prepared_global_data_authority.md
- ideas/open/609_rv64_global_data_consumer.md
- src/backend/prealloc/object_data.hpp
- src/backend/prealloc/object_data.cpp
- src/backend/prealloc/prepared_contract_verifier.cpp
- src/backend/prealloc/prepared_contract_verifier.hpp
- docs/prepared_fact_contracts/storage_initializer_contract_plan.md

## Current Targets

- Mixed selected global object-data row: `src/20010924-1.c`.
- Neighboring selected object-data rows: `src/pr61517.c`,
  `src/pr57877.c`, `src/pr57860.c`, and `src/20030224-2.c`.
- Prepared object-data records that need ordinary emitted bytes plus relocation
  slot offsets and target identity.

## Non-Goals

- RV64 relocation-record emission or object byte emission.
- RV64 global symbol materialization, access-width support, or section output.
- Prepared global memory facts and direct global-symbol base-plus-offset
  authority parked by `608`.
- BIR initializer bootstrap, unsupported-marker policy changes, expectations,
  allowlists, timeouts, runtime/link behavior, or accounting.

## Working Model

`PreparedGlobalObjectData` currently records whole-object bytes, zero-fill,
and relocation presence as booleans. That is enough for simple byte-only,
zero-fill, and relocation-only rows, but not for mixed aggregate data where
ordinary bytes and pointer relocation slots coexist. This runbook owns the
prepared representation and producer/contract verifier needed to describe that
mixed object safely. Once a record is coherent, RV64 may still fail later on
relocation-record or byte emission; that remains `609` consumer work.

## Execution Rules

- Start every code packet from captured prepared facts and current diagnostics,
  not from one representative testcase alone.
- Keep schema additions explicit: relocation slots must carry byte offset,
  size when known, and target identity.
- Preserve fail-closed diagnostics for missing symbols, missing offsets,
  overlapping bytes and relocation slots, unknown extent, contradictory
  initializer evidence, unsupported markers, and unresolved target identity.
- Do not move object emission or relocation-record production into RV64 target
  code as a substitute for prepared facts.
- Prove code changes with a fresh build plus the supervisor-selected narrow
  RV64 gcc-torture allowlist.
- Escalate to broader validation if the schema or verifier change affects
  byte-only, zero-fill, or relocation-only object-data rows outside the narrow
  mixed set.

## Ordered Steps

### Step 1: Inventory mixed object-data fact gap

Goal: capture the exact prepared facts and diagnostics for the mixed row and
neighboring selected object-data rows before changing schema or producer code.

Primary targets:
- `build/rv64_gcc_c_torture_backend/<case-id>/case.log`
- `src/backend/prealloc/object_data.hpp`
- `src/backend/prealloc/object_data.cpp`
- `src/backend/prealloc/prepared_contract_verifier.cpp`

Actions:
- Inspect `src/20010924-1.c` and at least two neighboring selected object-data
  rows from the current allowlist.
- Record current `PreparedGlobalObjectData` fields for object label, size,
  alignment, emitted bytes, zero fill, relocation booleans, unsupported marker,
  and contract status.
- Identify where BIR initializer evidence contains ordinary bytes and pointer
  relocation entries, including expected slot offsets and target symbols.
- Decide whether the first implementation packet should add schema, producer
  population, verifier checks, or a smaller prerequisite.
- Keep all evidence in `todo.md`; do not edit code in this inventory packet.

Completion check:
- `todo.md` names the first missing prepared fact, the representative rows,
  and the exact proof command for the implementation packet.

### Step 2: Add explicit relocation-slot prepared facts

Goal: extend prepared object-data facts so mixed records can represent
relocation slots without overloading whole-object relocation booleans.

Primary targets:
- `src/backend/prealloc/object_data.hpp`
- `src/backend/prealloc/prepared_contract_verifier.cpp`
- existing object-data helper tests or narrow compiler proof selected by the
  supervisor

Actions:
- Add a prepared relocation-slot record with byte offset, width or byte size
  when known, and target identity.
- Keep `requires_relocation` and `has_relocation` semantics compatible with
  existing relocation-only rows.
- Teach the verifier to reject missing, duplicate, overlapping, out-of-range,
  or targetless relocation slots.
- Preserve existing byte-only, zero-fill, unsupported, and relocation-only
  diagnostics.

Completion check:
- Schema and verifier changes build, existing relocation-only selected
  object-data progress is preserved, and no mixed row is marked coherent until
  producer data actually supplies slots and bytes.

### Step 3: Populate mixed bytes and relocation slots

Goal: populate coherent mixed prepared object-data from BIR initializer/global
layout evidence.

Primary targets:
- `src/backend/prealloc/object_data.cpp`
- BIR global initializer element fields that carry byte values and pointer
  symbol references

Actions:
- Convert initializer evidence into emitted byte spans plus relocation slots
  only when object size, alignment, slot offset, slot target, and ordinary byte
  ranges are known.
- Preserve fail-closed unsupported records for partial, ambiguous, TLS/GOT,
  thread-local, extern-only, or contradictory initializer forms.
- Avoid reconstructing target relocation records or section bytes in RV64.
- Keep the implementation semantic across the mixed family, not special-cased
  to `src/20010924-1.c`.

Completion check:
- At least one mixed selected object-data row moves past the prepared contract
  stop for the new prepared facts, and neighboring rows either move for the
  same rule or retain precise fail-closed diagnostics.

### Step 4: Prove handoff back to global-data consumers

Goal: record the split between coherent prepared mixed object data and later
RV64 object emission/relocation consumption.

Primary targets:
- mixed selected object-data allowlist
- `todo.md`
- `ideas/open/608_prepared_global_data_authority.md` if the supervisor asks
  for lifecycle closure or reactivation

Actions:
- Re-run the supervisor-selected narrow proof after implementation.
- Record rows that moved from the prepared selected object-data contract stop
  to a later RV64 consumer diagnostic.
- Confirm remaining RV64 relocation-record, byte emission, symbol
  materialization, and access-width diagnostics stay assigned to `609`.
- Return to the supervisor for lifecycle routing; do not close `608` unless
  delegated and the source idea criteria are satisfied.

Completion check:
- `todo.md` records the movement, remaining parked rows, and handoff owner.
  The source idea can be closed or reactivated only through a separate
  plan-owner close/route decision.
