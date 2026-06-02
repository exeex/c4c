# AArch64 Memory Frame-Slot Address Materialization Owner Runbook

Status: Active
Source Idea: ideas/open/88_aarch64_memory_frame_slot_address_materialization_owner.md

## Purpose

Split AArch64 memory-local frame-slot lookup and address materialization helpers
out of the broad memory lowering surface without changing selected memory
operand semantics.

## Goal

Create a narrow owner for frame-slot address materialization currently clustered
in `src/backend/mir/aarch64/codegen/memory.cpp` and exposed through
`src/backend/mir/aarch64/codegen/memory.hpp`.

## Core Rule

This is a behavior-preserving owner-boundary extraction. Preserve memory
lowering's authority over selected operands, AArch64 address spelling, base
policy, offset folding, scratch materialization, generated records,
diagnostics, and unsupported-path contracts.

## Read First

- `ideas/open/88_aarch64_memory_frame_slot_address_materialization_owner.md`
- `src/backend/mir/aarch64/codegen/memory.cpp`
- `src/backend/mir/aarch64/codegen/memory.hpp`
- `tests/backend/mir/backend_aarch64_prepared_memory_operand_records_test.cpp`
- `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`

## Current Targets

- `register_indirect_address`
- `fixed_slots_use_frame_pointer`
- `frame_slot_address`
- `find_frame_slot`
- `prepared_frame_slot_load_address`
- `prepared_frame_slot_memory_offset`
- `make_prepared_frame_slot_memory_operand`
- `materialize_frame_slot_memory_address_lines`
- frame-slot offset resolution and fixed-slot base application call sites
- memory-backed f128 and variadic consumers that already consume prepared
  frame-slot facts

## Non-Goals

- Do not move AArch64 offset legality, scratch selection, register spelling, or
  memory opcode choice into shared BIR or prealloc authority.
- Do not recreate local source-authority, value-home, storage-plan,
  stack-source, or frame-offset decisions already prepared by other owners.
- Do not move `va_list` field addressing or cursor update logic into this
  owner.
- Do not perform a broad mechanical `memory.cpp` split whose proof is only
  line-count reduction.
- Do not reopen dispatch publication, edge-copy, prepared-wrapper contraction,
  or store-retargeting decisions from adjacent ideas.

## Working Model

The new owner should stay AArch64-memory-local. It may consume prepared stack
layout, prepared memory access, value-home/storage facts, and frame-slot
offsets, but it must not decide those facts. Existing memory lowering should
continue to decide which memory operands are selected, when prepared facts are
available, which scratch registers are used, and which assembly opcode is
emitted.

## Execution Rules

- Start with a no-semantics mapping step before moving helpers.
- Prefer private helper movement first; widen the header surface only for
  existing AArch64 memory consumers.
- Keep memory lowering call sites explicit enough that ownership remains
  reviewable.
- Keep materialized-address lines and direct address spelling byte-for-byte
  equivalent unless a test proves the old spelling was already invalid.
- For every code-changing step, run a fresh build or compile proof plus the
  delegated narrow tests.
- Use matching `test_before.log` and `test_after.log` if the accepted slice
  crosses public headers, shared AArch64 memory call sites, or
  memory-backed f128/variadic consumers.

## Ordered Steps

### Step 1: Map the Existing Frame-Slot Address Boundary

Goal: identify the exact frame-slot/address helper cluster and consumers before
any movement.

Primary target: `src/backend/mir/aarch64/codegen/memory.cpp` and
`src/backend/mir/aarch64/codegen/memory.hpp`.

Concrete actions:

- Inspect each current target listed above and record which call sites need the
  helper after extraction.
- Classify helpers into address text formatting, fixed-slot base policy,
  prepared frame-slot lookup/offset folding, prepared memory operand creation,
  and emitted address materialization.
- Identify memory-backed f128 and variadic consumers that must remain consumers
  of prepared frame-slot facts rather than new owners of frame-slot policy.
- Decide the minimal owner file/header shape for the extraction.
- Do not edit implementation code in this step unless the executor is
  explicitly delegated a code-changing packet.

Completion check:

- `todo.md` names the chosen owner file/header shape, the helpers to move, the
  required call sites, any f128/variadic consumer touchpoints, and the narrow
  proof command.

### Step 2: Extract Address Text and Prepared Frame-Slot Operand Helpers

Goal: move address formatting, frame-slot lookup, base policy, offset folding,
and prepared frame-slot operand construction behind the new memory-local owner
without changing behavior.

Primary targets:

- `register_indirect_address`
- `fixed_slots_use_frame_pointer`
- `frame_slot_address`
- `find_frame_slot`
- `prepared_frame_slot_load_address`
- `prepared_frame_slot_memory_offset`
- `make_prepared_frame_slot_memory_operand`

Concrete actions:

- Create the owner implementation under the existing AArch64 codegen memory
  area.
- Move only helpers that consume prepared frame-slot/address facts or produce
  AArch64 frame-slot address spelling.
- Keep selected memory-record construction and validation in memory lowering.
- Preserve direct `[sp]`/`[x29]` spelling, byte offsets, fixed-slot
  frame-pointer policy, and failure behavior for missing prepared facts.

Completion check:

- Backend build proof passes.
- Focused AArch64 memory proof covers local frame-slot loads/stores, prepared
  frame-slot operand construction, offset folding, and fixed-slot base policy.

### Step 3: Extract Frame-Slot Address Materialization Lines

Goal: move emitted materialized frame-slot address lines into the same owner
while preserving scratch-register and address-spelling semantics.

Primary target: `materialize_frame_slot_memory_address_lines`.

Concrete actions:

- Move materialized frame-slot address emission only after Step 2 has preserved
  the direct address spelling helpers it shares.
- Keep scratch choice and opcode selection at the existing memory lowering
  boundary.
- Preserve immediate add/sub behavior, large-offset constant materialization,
  and non-frame-slot no-op behavior.
- Update existing memory-backed consumers to call the owner without changing
  their selected memory operand authority.

Completion check:

- Backend build proof passes.
- Focused proof covers materialized frame-slot addresses, large offsets,
  negative offsets, and unchanged direct frame-slot memory forms.

### Step 4: Check Memory-Backed f128 and Variadic Consumers

Goal: ensure touched memory-backed f128 and variadic paths still consume the
same prepared frame-slot facts without absorbing unrelated ABI logic.

Primary targets:

- memory-backed f128 load/store and transport call sites in `memory.cpp`
- variadic memory-backed call sites in `memory.cpp`
- prepared frame-slot offset resolution call sites

Concrete actions:

- Audit any f128 or variadic call site touched by the extraction.
- Keep `va_list` field addressing and cursor update logic outside the new
  owner.
- Keep f128 transport, ABI, scratch, and storage-plan decisions at their
  existing owners.
- Add or preserve focused tests only where an actually touched consumer needs
  proof.

Completion check:

- Focused proof covers every touched f128 or variadic memory-backed consumer.
- No ABI construction, `va_list` field addressing, or cursor update authority
  moved into the frame-slot owner.

### Step 5: Tighten Surface and Validate Acceptance

Goal: keep the owner boundary narrow and prove the split did not alter memory
lowering semantics.

Primary targets:

- new owner header/source
- `src/backend/mir/aarch64/codegen/memory.cpp`
- `src/backend/mir/aarch64/codegen/memory.hpp`
- relevant build registration files if a new source file is introduced

Concrete actions:

- Remove temporary public declarations not required by existing memory
  consumers.
- Verify the owner API exposes only frame-slot/address materialization
  authority and consumed prepared facts.
- Check generated records and diagnostics for unchanged frame-slot load/store,
  prepared operand, materialized address, fixed-slot base, and touched
  f128/variadic paths.

Completion check:

- Backend build proof passes.
- Narrow AArch64 memory/prepared operand proof passes.
- If public headers, shared AArch64 memory call sites, or memory-backed
  f128/variadic consumers changed, matching before/after regression logs exist
  as `test_before.log` and `test_after.log`.
