# AArch64 Variadic Machine Node Consumption Runbook

Status: Active
Source Idea: ideas/open/243_aarch64_variadic_machine_node_consumption.md
Activated from: ideas/open/243_aarch64_variadic_machine_node_consumption.md
Reactivated after closing prerequisite: ideas/closed/245_prepared_scalar_va_arg_access_plan.md

## Purpose

Consume prepared AArch64 variadic-entry facts in selected machine-node lowering
for `va_start`, scalar `va_arg`, aggregate `va_arg`, and `va_copy` without
rebuilding AAPCS64 ABI or frame-layout decisions in target codegen.

## Goal

Produce structured AArch64 machine-node records and printer output for
representative variadic helper paths from complete prepared/shared facts, or
stop with an exact lifecycle blocker naming any remaining missing prepared
storage, scratch, operand-home, or access-plan fact.

## Core Rule

AArch64 lowering may consume prepared variadic facts, but must not reconstruct
AAPCS64 `va_list` layout, register-save areas, overflow-area offsets, named
argument counts, operand homes, scalar access plans, or scratch-resource policy
locally.

## Read First

- `ideas/open/243_aarch64_variadic_machine_node_consumption.md`
- `ideas/closed/232_aarch64_variadic_function_entry_carriers.md`
- `ideas/closed/244_aarch64_variadic_prepared_storage_and_helper_authority.md`
- `ideas/closed/245_prepared_scalar_va_arg_access_plan.md`
- `src/backend/prealloc/prealloc.hpp`
- `src/backend/prealloc/prealloc.cpp`
- `src/backend/prealloc/prepared_printer.cpp`
- `src/backend/mir/aarch64/codegen/instruction.hpp`
- `src/backend/mir/aarch64/codegen/instruction.cpp`
- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- `src/backend/mir/aarch64/codegen/machine_printer.cpp`
- `src/backend/mir/aarch64/codegen/variadic.md`
- focused tests under `tests/backend/mir/` and variadic C cases under
  `tests/c/internal/abi/`

## Current Targets

- `PreparedVariadicEntryPlanFunction` storage and helper-resource facts.
- Helper operand-home records from the prepared/shared carriers closed in idea
  244.
- Scalar `va_arg` access-plan records from the prepared/shared carrier closed
  in idea 245.
- AArch64 helper-call dispatch that recognizes variadic helpers and must now
  consume prepared scalar `va_arg` authority.
- Selected machine-node records for scalar `va_arg`, then aggregate `va_arg`
  and `va_copy`.
- Printer support that emits only from structured operands and prepared/shared
  facts.
- Focused tests that distinguish prepared-carrier presence from real machine
  node consumption.

## Non-Goals

- Do not infer register-save slots, stack offsets, overflow bases, `va_list`
  layout, named GP/FP counts, operand homes, scalar access plans, or scratch
  allocation inside AArch64 codegen.
- Do not weaken fail-closed diagnostics or mark unsupported helper cases as
  supported without selected machine-node evidence.
- Do not claim support through prepared dump coverage alone.
- Do not broaden into global address, memory load/store, scalar cast, i128,
  binary128, atomic, intrinsic, inline-asm, callee-save slot-placement, or
  preserved-value extent work.
- Do not rewrite closed prepared-authority ideas 244 or 245 except for
  historical correction explicitly requested by the supervisor.

## Working Model

Idea 232 made variadic callee-entry metadata visible and guarded. Step 1 of
this idea found missing storage, scratch, and operand-home authority, so idea
244 supplied those prerequisites as prepared/shared facts. Step 2 then landed
`va_start` selected machine-node consumption. Step 3 stopped on the narrower
missing scalar `va_arg` access-plan fact, and idea 245 supplied that
prepared/shared prerequisite.

This reactivated runbook resumes at scalar `va_arg` consumption. AArch64
target lowering should consume `helper_operand_homes.va_arg.scalar_access_plan`
directly. If any helper still lacks a necessary structured fact, keep AArch64
fail-closed behavior and record the smallest missing prepared/shared fact as a
new lifecycle blocker.

## Execution Rules

- Keep routine packet progress and proof in `todo.md`.
- Resume with scalar `va_arg`, then aggregate `va_arg`, `va_copy`, and final
  validation.
- Add or strengthen focused tests before relying on broader backend
  validation.
- Treat expectation-only changes, fixture-name matching, unsupported
  downgrades, diagnostic-only rewrites, prepared-dump-only claims, or local ABI
  reconstruction as route drift.
- For every code-changing packet, prove with a build plus the
  supervisor-chosen focused AArch64 variadic subset. Escalate to broader
  backend validation after shared selected-node, printer, or prepared-consumer
  behavior changes.
- If complete storage, scratch, operand-home, or access-plan authority is still
  missing, stop at an explicit blocker rather than filling the gap in AArch64
  target codegen.

## Ordered Steps

### Step 1: Inspect Prepared Variadic Consumption Boundary

Status: Completed before prerequisite split.

Goal: Determine which prepared variadic facts are already complete enough for
machine-node consumption and which helper paths still need new prepared/shared
authority.

Completion check:

- Completed by splitting and closing prerequisite idea 244. Do not redo this
  step unless a later packet finds that the closed prepared/shared facts are
  insufficient or stale.

### Step 2: Consume Prepared Facts For `va_start`

Status: Completed in commit `6009e54a9`.

Goal: Lower a representative `va_start` path into selected machine nodes from
prepared entry, helper-resource, and operand-home facts.

Completion check:

- Completed by selected `va_start` machine-node records and proof from prepared
  facts. Do not redo this step unless a scalar `va_arg` packet proves the
  shared variadic helper boundary needs a targeted correction.

### Step 3: Consume Prepared Facts For Scalar `va_arg`

Goal: Lower selected scalar `va_arg` paths without target-local ABI
reconstruction.

Actions:

- Add typed machine-node records for GP, FP, and overflow-backed scalar
  argument fetches.
- Consume prepared register-save-area storage, overflow-area progression,
  helper resources, source `va_list` homes, scalar result homes, and
  `helper_operand_homes.va_arg.scalar_access_plan`.
- Preserve value-width, alignment, source coordinates, `va_list` progression,
  and destination/result-home facts as structured operands.
- Add tests that cover at least one register-backed and one overflow-backed
  scalar access, or record the exact missing prepared/shared fact.
- Preserve explicit fail-closed diagnostics for missing or incomplete prepared
  scalar access-plan authority.

Completion check:

- Scalar `va_arg` lowering either emits structured machine records and printer
  output from prepared/shared facts or stops with a narrow blocker.

### Step 4: Consume Prepared Facts For Aggregate `va_arg`

Goal: Lower aggregate `va_arg` helper effects while preserving aggregate
storage and copy semantics.

Actions:

- Add records for aggregate payload source selection and destination copy.
- Consume prepared aggregate size, alignment, destination payload homes,
  source `va_list` homes, register-save access, overflow progression, and
  helper resources.
- Keep full-width aggregate transport separate from scalar shortcuts.
- Add focused tests for an aggregate variadic argument path.

Completion check:

- Aggregate `va_arg` either produces structured machine-node effects from
  prepared/shared facts or records an exact prepared extent/storage blocker.

### Step 5: Consume Prepared Facts For `va_copy`

Goal: Lower `va_copy` as a structured copy of the prepared `va_list` layout.

Actions:

- Add machine-node records for copying each supported `va_list` field.
- Consume prepared source/destination homes, field layout facts, storage facts,
  and helper resources directly.
- Preserve explicit diagnostics for incomplete storage or layout facts.
- Add focused tests for `va_copy` record and printer output.

Completion check:

- `va_copy` emits structured copy effects from prepared/shared facts, or the
  route records the exact missing prepared source/destination storage
  authority.

### Step 6: Validate And Summarize

Goal: Prove the accepted consumption route and preserve any remaining blockers
at the correct lifecycle layer.

Actions:

- Run the supervisor-chosen build and focused variadic backend subset.
- Escalate to broader `backend_` validation if selected machine records,
  printer behavior, or prepared facts changed beyond one helper family.
- Summarize supported helper paths, remaining unsupported cases, and any split
  prepared/shared initiatives in `todo.md`.
- Ask the supervisor to decide whether the source idea is complete, blocked, or
  should remain active for another runbook rewrite.

Completion check:

- Supported variadic helper paths have structured selected machine-node and
  printer proof, and remaining gaps are explicit non-overfit lifecycle notes.
