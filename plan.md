# AArch64 Memory Prepared Address Authority Cleanup Runbook

Status: Active
Source Idea: ideas/open/70_aarch64_memory_prepared_address_authority_cleanup.md

## Purpose

Make AArch64 memory lowering consume prepared addressing, value-home, storage,
frame-offset, aggregate stack-source, and typed stack-source authority before it
selects target-local load/store forms and address materialization sequences.

## Goal

Replace duplicated source-authority decisions in the AArch64 memory path with
lookups from the existing prepared facts while preserving AArch64 instruction
selection policy.

## Core Rule

Prepared facts decide what address, home, storage, frame offset, or stack source
is authoritative; AArch64 code decides only whether that fact can be encoded
directly and which registers, opcodes, and assembly spelling to use.

## Read First

- `ideas/open/70_aarch64_memory_prepared_address_authority_cleanup.md`
- `src/backend/mir/aarch64/codegen/memory.cpp`
- `src/backend/prealloc/addressing.*`
- `src/backend/prealloc/storage_plans.*`
- `src/backend/prealloc/value_locations.hpp`
- `src/backend/prealloc/prepared_lookups.*`

Read these consumers before touching them:

- `src/backend/mir/aarch64/codegen/f128.cpp`
- `src/backend/mir/aarch64/codegen/variadic.cpp`

## Current Targets

- AArch64 memory helpers that currently re-derive address sources, value homes,
  storage plans, frame offsets, aggregate stack sources, or typed stack-source
  publication.
- Local memory-backed transfer consumers in `f128.cpp` and `variadic.cpp` only
  where they cross the memory authority boundary.
- Shared prepared lookup call sites only where existing facts need to be
  consumed more directly.

## Non-Goals

- Do not invent a new aggregate transport planner.
- Do not move AArch64 offset legality, scratch-register selection, memory opcode
  selection, or address spelling into shared prealloc.
- Do not split files mechanically unless the split removes duplicated authority.
- Do not rewrite f128 or variadic lowering beyond direct consumption of
  memory-backed prepared facts.
- Do not weaken tests or mark supported memory paths unsupported.

## Working Model

- `PreparedAddressingFunction`, `PreparedMemoryAccess`, and
  `PreparedAddressMaterialization` describe address authority before target
  encoding.
- `PreparedValueHomeLookups`, `PreparedValueLocationFunction`, storage plans,
  and decoded home storage describe where values live before AArch64 emits
  loads, stores, copies, or address materialization.
- Frame-address offset queries and prepared stack-source authorities should be
  consumed as facts, not reconstructed from local MIR shape.
- AArch64 may still select immediate vs scratch address forms, temporary
  registers, load/store opcodes, and va_list field addressing locally.

## Execution Rules

- Work in narrow, reviewable steps and keep each implementation packet tied to
  one authority boundary.
- Prefer semantic prepared-fact consumption over testcase-shaped matching.
- When replacing a local derivation, leave evidence in code or tests that the
  prepared lookup is now the source of truth.
- If a needed prepared fact is missing or ambiguous, stop and record the blocker
  in `todo.md` instead of inventing a target-local substitute.
- Each code-changing step needs fresh build or compile proof; broader CTest or
  regression-guard proof is required when a packet changes shared prepared
  lookup behavior or multiple AArch64 consumers.

## Ordered Steps

### Step 1: Map Memory Authority Duplication

Goal: identify the exact AArch64 memory decisions that duplicate prepared
address, home, storage, frame-offset, or stack-source authority.

Primary target: `src/backend/mir/aarch64/codegen/memory.cpp`

Actions:

- Inspect memory helper entry points that form load/store addresses, materialize
  addresses, decode homes, or select stack/frame sources.
- Cross-reference each local decision with available prepared facts in
  `addressing.*`, `storage_plans.*`, `value_locations.hpp`, and
  `prepared_lookups.*`.
- Classify each duplicated decision as prepared-addressing, value-home/storage,
  frame-offset, aggregate stack-source, typed stack-source, or target-local
  instruction policy.
- Record the first executable packet in `todo.md`, including the proof command
  the supervisor delegates.

Completion check:

- `todo.md` names the first concrete implementation packet and distinguishes
  shared authority consumption from AArch64-local instruction policy.

### Step 2: Consume Prepared Addressing And Materialization Facts

Goal: make AArch64 memory address formation use prepared address/access facts
before selecting direct, scratch, local, global, pointer, or frame forms.

Primary target: `src/backend/mir/aarch64/codegen/memory.cpp`

Actions:

- Replace local address-source derivation with `PreparedAddressingFunction`,
  `PreparedMemoryAccess`, and `PreparedAddressMaterialization` lookups where
  those facts already exist.
- Preserve AArch64 offset encodability checks, scratch register choice, and
  final address spelling locally.
- Add or update focused tests for local, global, pointer, and frame-backed
  addressing forms touched by the change.

Completion check:

- Memory lowering consults prepared addressing/materialization facts before
  AArch64 instruction selection, and the focused AArch64 proof is green.

### Step 3: Fold Value-Home, Storage, And Frame-Offset Authority Into Memory

Goal: make memory load/store helpers consume prepared value-home, value-location,
storage-plan, decoded home-storage, and frame-offset facts.

Primary target: `src/backend/mir/aarch64/codegen/memory.cpp`

Actions:

- Replace local value-home and storage reconstruction with
  `PreparedValueHomeLookups`, `PreparedValueLocationFunction`, storage plans,
  decoded home storage, and frame-address offset queries.
- Keep target-local register class, load/store opcode, and immediate legality
  policy in AArch64 code.
- Prove local, frame, and memory-backed value-home cases that would expose
  accidental re-derivation.

Completion check:

- The memory path treats prepared home/storage/frame facts as authoritative and
  the relevant AArch64 subset remains green.

### Step 4: Consume Prepared Stack-Source Authorities

Goal: make aggregate and typed stack-source memory paths consume prepared
stack-source publication facts instead of reconstructing source authority.

Primary targets:

- `src/backend/mir/aarch64/codegen/memory.cpp`
- shared prepared lookup call sites only if an existing fact is not directly
  consumable

Actions:

- Route aggregate stack-source decisions through
  `PreparedAggregateStackSourceAuthority`.
- Route typed stack-source decisions through
  `PreparedTypedStackSourcePublication`.
- Keep aggregate transport planning out of this runbook; only consume existing
  stack-source facts.
- Prove nearby aggregate and typed stack-source memory cases, not just one
  named testcase.

Completion check:

- Stack-source memory lowering no longer locally re-derives prepared source
  authority, and focused stack-source tests are green.

### Step 5: Align Memory-Backed f128 And Variadic Consumers

Goal: update local AArch64 consumers that cross the memory authority boundary so
they consume the same prepared facts as the main memory path.

Primary targets:

- `src/backend/mir/aarch64/codegen/f128.cpp`
- `src/backend/mir/aarch64/codegen/variadic.cpp`

Actions:

- Inspect only memory-backed f128 and variadic transfer paths.
- Replace duplicated memory authority decisions with calls into the prepared
  fact consumption path established by earlier steps.
- Preserve f128-specific and variadic-specific ABI decisions, including va_list
  field addressing, in their local code.
- Add or update focused coverage when either file changes.

Completion check:

- f128 and variadic memory-backed paths share prepared authority consumption
  with memory lowering, and their focused proof remains green.

### Step 6: Acceptance Validation And Drift Check

Goal: prove the cleanup behaves as a general authority repair rather than a
narrow testcase adjustment.

Actions:

- Run the supervisor-selected focused AArch64 memory-lowering subset.
- Run broader regression-guard proof for acceptance-sized slices.
- Inspect the final diff for rejected patterns from the source idea: local
  re-derivation under new helper names, expectation downgrades, unsupported
  rewrites, or target-specific policy moved into shared prealloc.
- Record proof results in `todo.md`.

Completion check:

- Build and test proof are fresh, the diff still matches the source idea, and
  the supervisor has enough evidence to decide whether the source idea can be
  closed or needs another runbook.
