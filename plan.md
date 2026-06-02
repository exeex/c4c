# AArch64 Memory Store-Retargeting Owner Runbook

Status: Active
Source Idea: ideas/open/89_aarch64_memory_store_retargeting_owner.md

## Purpose

Split AArch64 memory-local store-retargeting helpers out of the broad memory
lowering surface without changing selected memory-record semantics.

## Goal

Create a narrow owner for store-retargeting and stack-layout rewrites currently
clustered in `src/backend/mir/aarch64/codegen/memory.cpp` and exposed through
`src/backend/mir/aarch64/codegen/memory.hpp`.

## Core Rule

This is an owner-boundary extraction. Preserve behavior, diagnostics,
unsupported-path contracts, selected memory-record fields, and existing memory
lowering entry points.

## Read First

- `ideas/open/89_aarch64_memory_store_retargeting_owner.md`
- `src/backend/mir/aarch64/codegen/memory.cpp`
- `src/backend/mir/aarch64/codegen/memory.hpp`
- `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`
- `tests/backend/mir/backend_aarch64_prepared_memory_operand_records_test.cpp`

## Current Targets

- `store_local_uses_pointer_value_address`
- `prepared_or_emitted_store_value_register`
- `retarget_pointer_store_value_to_materialized_address`
- `retarget_store_address_to_materialized_pointer`
- `retarget_pointer_store_value_to_emitted_scalar`
- `retarget_store_local_value_to_emitted_scalar`
- `find_prepared_local_address_store_value`
- `rewrite_local_address_store_value`
- `apply_stack_layout_to_memory_record`

## Non-Goals

- Do not publish generic storage, register, diagnostic, dispatch, BIR, or
  stack-layout authority.
- Do not move store-local/store-global publication planning, edge-copy
  publication, or prepared-wrapper contraction into this owner.
- Do not weaken diagnostics, unsupported-path behavior, selected record
  contracts, or test expectations.
- Do not perform a broad memory-record builder extraction beyond the
  store-retargeting rewrite boundary.

## Working Model

The new owner should stay AArch64-memory-local. It may consume prepared pointer
facts, frame-slot facts, emitted scalar state, and prepared stack layout, but it
must not recreate or widen those authorities. Existing memory lowering should
continue to decide when memory records are selected and when the retargeting
owner is invoked.

## Execution Rules

- Keep the first implementation step as an audit or no-semantics extraction.
- Prefer private helper movement first; widen the header surface only when an
  existing AArch64 memory consumer already requires that symbol.
- Keep call sites in memory lowering explicit enough that ownership remains
  reviewable.
- For every code-changing step, run a fresh build or compile proof plus the
  delegated narrow tests.
- Use matching `test_before.log` and `test_after.log` if the accepted slice
  crosses public headers or shared AArch64 memory call sites.

## Ordered Steps

### Step 1: Map the Existing Retargeting Boundary

Goal: identify the exact helper cluster and callers before any movement.

Primary target: `src/backend/mir/aarch64/codegen/memory.cpp` and
`src/backend/mir/aarch64/codegen/memory.hpp`.

Concrete actions:

- Inspect each current target listed above and record which call sites need the
  helper after extraction.
- Classify helpers into pointer store-value/address retargeting,
  emitted-scalar store reuse, local-address store-value rewrite, and
  stack-layout application.
- Decide the minimal owner file/header shape for the extraction.
- Do not edit implementation code in this step unless the executor is
  explicitly delegated a code-changing packet.

Completion check:

- `todo.md` names the chosen owner file/header shape, the helpers to move, the
  required call sites, and the narrow proof command.

### Step 2: Extract Pointer Store Retargeting

Goal: move pointer store-value/address retargeting behind the new memory-local
owner without changing behavior.

Primary targets:

- `retarget_pointer_store_value_to_materialized_address`
- `retarget_store_address_to_materialized_pointer`
- `retarget_pointer_store_value_to_emitted_scalar`
- `retarget_store_local_value_to_emitted_scalar`
- `store_local_uses_pointer_value_address`
- `prepared_or_emitted_store_value_register`

Concrete actions:

- Create the owner implementation under the existing AArch64 codegen memory
  area.
- Move only the retargeting helpers needed by existing memory lowering.
- Keep memory lowering responsible for when the helpers are called.
- Preserve all memory-record field resets, value replacement rules, and
  prepared/emitted scalar lookup order.

Completion check:

- Backend build proof passes.
- Focused AArch64 memory/dispatch tests covering pointer-address stores,
  materialized-address retargeting, and emitted-scalar store reuse pass.

### Step 3: Extract Stack-Layout Store Record Rewrites

Goal: move local-address store-value rewrite and stack-layout application into
the same owner if the Step 1 boundary proves they are part of the
store-retargeting responsibility.

Primary targets:

- `find_prepared_local_address_store_value`
- `rewrite_local_address_store_value`
- `apply_stack_layout_to_memory_record`

Concrete actions:

- Move the local-address rewrite helpers only after preserving the frame-slot
  and prepared-address inputs they consume.
- Keep frame-slot lookup and stack-layout facts as consumed authority, not
  recreated authority.
- Keep non-store and no-local-store paths returning the same success/failure
  values.

Completion check:

- Focused proof covers local-address store-value rewrite and stack-layout
  application to memory records.
- Existing prepared memory operand record tests remain green.

### Step 4: Tighten Surface and Validate Acceptance

Goal: make the owner boundary narrow and prove the split did not alter memory
lowering semantics.

Primary targets:

- new owner header/source
- `src/backend/mir/aarch64/codegen/memory.cpp`
- `src/backend/mir/aarch64/codegen/memory.hpp`
- relevant build registration files if a new source file is introduced

Concrete actions:

- Remove any temporary public declarations that are not required by existing
  memory consumers.
- Verify the owner API does not expose generic storage, dispatch, register, or
  diagnostic authority.
- Check generated records and diagnostics for unchanged pointer-address,
  emitted-scalar, local-address, and stack-layout rewrite paths.

Completion check:

- Backend build proof passes.
- Narrow AArch64 memory/dispatch proof passes.
- If public headers or shared AArch64 memory call sites changed, matching
  before/after regression logs exist as `test_before.log` and
  `test_after.log`.
