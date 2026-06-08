# AArch64 Va List Field Memory Local Owner Runbook

Status: Active
Source Idea: ideas/open/126_aarch64_memory_va_list_field_local_owner.md

## Purpose

Make the AArch64-local `va_list` field memory route easier to own and review
without changing prepared variadic authority or ordinary memory lowering
behavior.

## Goal

Extract or isolate the `va_list` field memory helper cluster currently embedded
in `src/backend/mir/aarch64/codegen/memory.cpp` behind a clear AArch64-local
owner or private helper boundary.

## Core Rule

Preserve behavior. This work is a local ownership cleanup for AArch64 memory
lowering, not a shared prepared/BIR authority migration and not a variadic
semantic rewrite.

## Read First

- `ideas/open/126_aarch64_memory_va_list_field_local_owner.md`
- `src/backend/mir/aarch64/codegen/memory.cpp`
- `src/backend/mir/aarch64/codegen/memory.hpp`
- `src/backend/mir/aarch64/codegen/variadic.cpp` only for include fallout or
  current route understanding

## Current Targets

- The current helper cluster:
  - `parse_va_list_field_suffix`
  - `VaListFieldAddress`
  - `find_va_list_field_address`
  - `make_va_list_field_memory_operand`
  - `make_va_list_field_load_memory_instruction_record`
  - `make_va_list_field_store_memory_instruction_record`
  - `va_list_field_cursor_update_producer`
  - `make_va_list_field_cursor_update_machine_instruction`
- `lower_memory_instruction` as the consumer that asks the local owner whether
  a load/store should use the special `va_list` field route.
- Build metadata if a new private translation unit is introduced.
- Focused backend/AArch64 tests that exercise the touched route.

## Non-Goals

- Do not move variadic entry planning, helper operand homes, `va_list` layout,
  value-home facts, or storage-plan facts out of shared prepared code.
- Do not move AArch64 ABI field offsets, register spelling, scratch policy, or
  memory operand emission into shared code.
- Do not rework `src/backend/mir/aarch64/codegen/variadic.cpp` except for
  genuine include fallout.
- Do not reopen ideas 57, 102, 112, 121, 124, or 125.
- Do not perform broad `memory.cpp` splitting or line-count-only cleanup.
- Do not weaken tests, mark supported variadic paths unsupported, or rewrite
  expectations to hide changed memory behavior.

## Working Model

Shared prepared/BIR code owns prepared facts: variadic entry plans, `va_list`
layout fields, helper operand homes, value homes, and storage plans.

AArch64 memory lowering owns how those prepared facts become target memory
operands, field load/store records, cursor-update machine instructions,
scratch-register choices, and target diagnostics.

The new owner should make that boundary explicit while keeping fallback to
ordinary prepared memory lowering intact when a slot is not a prepared
`va_list` field.

## Execution Rules

- Keep each step narrow enough to validate with build proof and focused
  backend/AArch64 tests.
- Prefer a private helper surface over widening `memory.hpp`.
- If a new file is added, keep declarations private to the AArch64 memory
  codegen area unless a narrow public declaration is required.
- Preserve the existing diagnostics and final machine instruction records
  unless the source idea explicitly requires a better local factoring.
- Escalate to broader backend validation if shared headers, memory record
  construction, or non-`va_list` memory routes are touched.
- Treat testcase-shaped shortcuts and expectation downgrades as route failures,
  not progress.

## Ordered Steps

### Step 1: Characterize the Current Route

Goal: Identify the existing load, store, cursor-update, and fallback paths
before any movement.

Primary target: `src/backend/mir/aarch64/codegen/memory.cpp`

Actions:

- Inspect how `lower_memory_instruction` reaches the `va_list` field helper
  cluster.
- Trace which prepared facts each helper consumes and confirm no helper owns
  shared variadic selection.
- Identify focused tests covering variadic `va_start` / `va_arg`, `va_list`
  field loads, `va_list` field stores, cursor-update store emission, and
  ordinary memory fallback.
- Record any missing or weak focused coverage in `todo.md` for the executor
  packet.

Completion check:

- The executor can name the exact helper cluster, current callers, relevant
  prepared facts, and focused proof command before changing code.

### Step 2: Choose the Local Owner Boundary

Goal: Decide the smallest AArch64-local owner surface that isolates the helper
cluster without widening shared authority.

Primary targets:

- `src/backend/mir/aarch64/codegen/memory.cpp`
- `src/backend/mir/aarch64/codegen/memory.hpp`
- optional new private AArch64 memory helper file/header

Actions:

- Choose whether the owner remains a private namespace surface in `memory.cpp`
  or moves into a new private helper file/header.
- Keep `lower_memory_instruction` as the consumer of a local query/emission
  surface for special `va_list` field routing.
- Define the owner API around prepared facts and AArch64-local memory emission,
  not around recomputing variadic layout or homes.
- Avoid exporting broad internals through `memory.hpp`.

Completion check:

- The chosen boundary has a narrow caller shape and does not require shared
  prepared/BIR code to depend on AArch64 memory helpers.

### Step 3: Extract or Isolate the Helper Cluster

Goal: Move or group the `va_list` field helper cluster behind the chosen
owner while preserving machine-record behavior.

Primary target: the chosen AArch64-local memory owner surface.

Actions:

- Move or group the parsing, address lookup, memory operand, load/store record,
  cursor-update producer, and cursor-update instruction helpers together.
- Keep existing prepared fact consumption and AArch64 register conversion
  behavior intact.
- Update build metadata if a new translation unit is introduced.
- Keep include fallout minimal and local.

Completion check:

- The code builds, the helper cluster has one clear AArch64-local owner, and
  no new shared authority or broad public declaration was introduced.

### Step 4: Reconnect `lower_memory_instruction` and Fallback

Goal: Keep ordinary memory lowering behavior equivalent while routing only
prepared `va_list` field loads/stores through the local owner.

Primary target: `lower_memory_instruction`

Actions:

- Make `lower_memory_instruction` ask the local owner whether special
  `va_list` field handling applies.
- Preserve fallback to ordinary prepared memory lowering when special handling
  does not apply.
- Verify diagnostics and instruction records remain equivalent for both the
  special route and fallback route.

Completion check:

- Focused tests demonstrate special `va_list` handling still works and ordinary
  memory fallback is not captured by the special route.

### Step 5: Focused Proof and Broader Check Decision

Goal: Prove the cleanup and decide whether broader validation is required.

Primary targets:

- focused backend/AArch64 variadic tests
- existing regression cases around `00204` or nearby variadic aggregate
  coverage when they exercise the route

Actions:

- Run build proof after code movement.
- Run focused backend/AArch64 tests covering `va_start` / `va_arg` memory
  interaction, `va_list` field loads, field stores, cursor-update store
  emission, and ordinary fallback.
- Include existing `00204` or nearby variadic aggregate regression coverage if
  it reaches the touched route.
- Escalate to broader backend validation if shared headers or memory record
  construction outside the local owner changed.

Completion check:

- Focused tests pass without expectation downgrades, and the supervisor has a
  clear validation recommendation for commit acceptance.
