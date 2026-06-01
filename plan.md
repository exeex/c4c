# AArch64 Variadic Prepared Entry Plan Cleanup Runbook

Status: Active
Source Idea: ideas/open/73_aarch64_variadic_prepared_entry_plan_cleanup.md

## Purpose

Make AArch64 variadic lowering consume prepared variadic entry-plan and helper
operand-home facts directly before emitting target-local variadic ABI records.

## Goal

AArch64 variadic code should use prepared shared facts for entry plans and
helper operand homes, while keeping va_list layout, save-area instruction
selection, va_arg/va_copy record emission, and concrete ABI addressing in the
AArch64 target layer.

## Core Rule

Do not reconstruct variadic entry-plan or helper operand-home policy in
`src/backend/mir/aarch64/codegen/variadic.cpp` when prepared facts already
exist. Keep target ABI record construction local.

## Read First

- `ideas/open/73_aarch64_variadic_prepared_entry_plan_cleanup.md`
- `src/backend/mir/aarch64/codegen/variadic.cpp`
- `src/backend/prealloc/variadic_entry_plans.*`
- `src/backend/prealloc/variadic.hpp`
- prepared value-home and addressing lookup surfaces used by AArch64 variadic
  lowering

## Current Targets

- `src/backend/mir/aarch64/codegen/variadic.cpp`
- Shared prepared authority call sites only where needed to consume existing
  facts:
  - `src/backend/prealloc/variadic_entry_plans.*`
  - `src/backend/prealloc/variadic.hpp`
  - relevant prepared value-home/addressing lookup surfaces

## Non-Goals

- Do not move AArch64 va_list layout spelling or save-area instruction
  selection into shared prealloc code.
- Do not combine this route with i128/f128 helper policy cleanup.
- Do not change variadic ABI behavior to make one testcase pass.
- Do not perform general memory address authority cleanup beyond the prepared
  facts needed by variadic lowering.
- Do not weaken variadic tests, supported markers, diagnostics, or expected
  behavior without explicit approval.

## Working Model

- Shared prealloc owns prepared variadic entry-plan and helper operand-home
  authority.
- AArch64 owns concrete ABI lowering: GP/FP save-area instruction records,
  va_arg and va_copy records, va_list layout spelling, and save-area
  addressing.
- A valid slice removes local policy reconstruction or proves that a remaining
  local decision is target-specific emission.

## Execution Rules

- Prefer small behavior-preserving packets.
- Before changing code, inventory the existing prepared facts and local
  reconstruction points.
- If a prepared fact is missing, record the blocker in `todo.md` rather than
  fabricating AArch64-local policy.
- Keep helper names and data flow traceable to the prepared authority consumed.
- For each code-changing step, run a fresh build or compile proof plus the
  focused variadic subset chosen by the supervisor.
- Regression guard logs are required before acceptance-sized slices are
  treated as complete.

## Ordered Steps

### Step 1: Inventory Prepared Variadic Authority

Goal: map the current AArch64 variadic lowering path against the prepared
entry-plan and helper operand-home facts.

Primary targets:
- `src/backend/mir/aarch64/codegen/variadic.cpp`
- `src/backend/prealloc/variadic_entry_plans.*`
- `src/backend/prealloc/variadic.hpp`

Actions:
- Inspect how `PreparedVariadicEntryPlanFunction` is produced and exposed.
- Inspect how `PreparedVariadicEntryHelperOperandHomes` is produced and
  queried.
- Identify each AArch64 variadic site that reconstructs entry-plan policy,
  helper operand homes, or prepared value-home/addressing decisions.
- Classify remaining local decisions as either prepared-policy reconstruction
  or target-local ABI emission.
- Record the inventory and first safe implementation target in `todo.md`.

Completion check:
- `todo.md` names the prepared facts available, the local reconstruction sites,
  and the next code-changing packet.
- No implementation files are changed in this step unless the supervisor
  explicitly delegates a combined inventory-and-edit packet.

### Step 2: Consume Prepared Entry Plans

Goal: route AArch64 variadic entry lowering through the prepared variadic
entry-plan fact before target-local record construction.

Primary target:
- `src/backend/mir/aarch64/codegen/variadic.cpp`

Actions:
- Replace local entry-plan policy reconstruction with direct consumption of
  `PreparedVariadicEntryPlanFunction` where the prepared fact is available.
- Preserve AArch64 save-area record construction, ABI layout spelling, and
  concrete GP/FP save-area instruction selection.
- If lookup plumbing is needed, keep it at the prepared-authority boundary and
  avoid broad prealloc redesign.
- Record any missing prepared fact as a blocker instead of adding a fallback
  that re-derives the same policy.

Completion check:
- AArch64 variadic entry lowering consumes the prepared entry plan.
- Focused build or compile proof passes.
- Focused AArch64 variadic entry/save-area tests or the delegated CTest subset
  pass.

### Step 3: Consume Prepared Helper Operand Homes

Goal: route helper operand-home decisions through
`PreparedVariadicEntryHelperOperandHomes` before helper-specific target
lowering.

Primary targets:
- `src/backend/mir/aarch64/codegen/variadic.cpp`
- prepared value-home/addressing lookup surfaces only if needed

Actions:
- Replace local helper operand-home policy reconstruction with prepared
  helper-home queries.
- Keep AArch64 register, stack, address, and instruction-record emission local.
- Preserve existing target-specific helper call and operand formatting
  behavior.
- Add blocker notes in `todo.md` for any helper path that lacks a prepared
  authority fact.

Completion check:
- Helper operand-home decisions are read from prepared facts for the migrated
  paths.
- Focused build or compile proof passes.
- Focused helper operand-home coverage or the delegated CTest subset passes.

### Step 4: Validate va_arg And va_copy Consumption Boundaries

Goal: prove va_arg and va_copy lowering consume prepared variadic facts without
moving target-local ABI emission into shared code.

Primary target:
- `src/backend/mir/aarch64/codegen/variadic.cpp`

Actions:
- Trace va_arg and va_copy record emission after the Step 2 and Step 3 data
  flow changes.
- Remove any remaining prepared-policy reconstruction found in those paths.
- Preserve va_arg/va_copy record schemas, va_list layout spelling, and concrete
  instruction selection locally.
- Extend focused proof only where the migrated data flow needs coverage.

Completion check:
- va_arg and va_copy paths consume prepared entry/helper facts where relevant.
- Focused va_arg and va_copy proof passes.
- Any unsupported or missing-authority cases are explicitly recorded in
  `todo.md`.

### Step 5: Acceptance Validation And Cleanup

Goal: finish the route with no local prepared-policy reconstruction and with
acceptance-level proof.

Actions:
- Re-read `variadic.cpp` and the touched prepared surfaces for remaining
  renamed reconstruction of entry plans or helper operand homes.
- Remove dead local helpers made obsolete by prepared fact consumption.
- Run the supervisor-chosen focused variadic subset covering entry plans,
  helper operand homes, GP/FP save areas, va_arg, and va_copy.
- Produce or refresh canonical regression guard logs when the supervisor asks
  for acceptance proof.

Completion check:
- The active source idea's proof expectations are satisfied.
- No target-local ABI emission was moved into shared prealloc code.
- `todo.md` records final proof commands and any residual follow-up that should
  become a separate idea instead of expanding this route.
