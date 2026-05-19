# AArch64 Selected Call-Boundary Move Preparation Printing Runbook

Status: Active
Source Idea: ideas/open/311_aarch64_selected_call_boundary_move_preparation_printing.md
Activated from: ideas/open/295_backend_regex_failure_family_inventory.md Step 4 split

## Purpose

Execute the focused owner split from umbrella idea 295 for AArch64 selected
call-boundary move preparation and printer admission.

## Goal

Make supported prepared call-boundary move records selected and printable only
when they carry structured prepared source and destination facts.

## Core Rule

Do not improve this route by suppressing diagnostics, weakening tests, or
marking call-boundary move records selected without printable prepared
source/destination data. Progress must repair the semantic handoff from
prepared call-boundary facts to selected AArch64 machine nodes.

## Read First

- `ideas/open/311_aarch64_selected_call_boundary_move_preparation_printing.md`
- `todo.md`
- `test_after.log` for the current `00140.c` frontend/printer failure
- `tests/c/external/c-testsuite/src/00140.c`
- `src/backend/mir/aarch64/codegen/calls.cpp`
- `src/backend/mir/aarch64/codegen/calls.hpp`
- `src/backend/mir/aarch64/codegen/instruction.hpp`
- `src/backend/mir/aarch64/codegen/machine_printer.cpp`
- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- `tests/backend/mir/backend_aarch64_target_instruction_records_test.cpp`
- `tests/backend/mir/backend_aarch64_machine_printer_test.cpp`
- `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`

## Current Scope

- AArch64 `CallBoundaryMoveInstructionRecord` preparation and selection
  status.
- Prepared source and destination facts from value homes, call argument plans,
  ABI bindings, and value-move bundles.
- Machine-printer handling for selected call-boundary moves.
- Focused proof targets:
  `backend_aarch64_target_instruction_records`,
  `backend_aarch64_machine_printer`,
  `backend_aarch64_instruction_dispatch`, and
  `c_testsuite_aarch64_backend_src_00140_c`.

## Non-Goals

- No expectation, allowlist, unsupported-classification, CTest registration,
  runner, timeout-policy, proof-log, or test-contract edits.
- No diagnostic suppression or selected-printer gate bypass.
- No filename, argument-index, `struct foo`, source-shape, or emitted-mnemonic
  shortcuts.
- No reopening closed owners 285 through 310 from counts alone.
- No direct-call shuffle, direct vararg, address-of-local, runtime
  nonzero/mismatch/crash, timeout/output-storm, unrelated machine-printer, or
  semantic `lir_to_bir` repairs under this focused owner.

## Working Model

- The observed failure is before assembly emission: no
  `build/c_testsuite_aarch64_backend/src/00140.c.s` exists.
- The printer sees a call-boundary move machine node whose selection status is
  `DeferredUnsupported` with the diagnostic that the node is outside the
  selected register call-boundary move subset.
- The producer path already has a semantic gate through
  `call_boundary_move_selection_status`. The repair should make the right
  prepared records satisfy that gate, while preserving fail-closed behavior
  for unsupported or incomplete records.
- `00140.c` is pressure evidence for prepared call-boundary moves involving
  aggregate/pointer/variadic direct-call setup. It is not the implementation
  key.

## Execution Rules

- Start with backend record and printer coverage before relying on the
  c-testsuite probe.
- Keep `todo.md` updated with the current packet, proof command, and observed
  residuals.
- Preserve fail-closed diagnostics for unselected, unsupported, or incomplete
  call-boundary move nodes.
- If localization proves a prerequisite producer-fact owner outside AArch64
  selected call-boundary move preparation, stop and ask the supervisor to
  split lifecycle state instead of broadening this plan.
- Use the supervisor-provided proof scope unless the supervisor delegates a
  broader validation command.

## Ordered Steps

### Step 1: Localize Selected Call-Boundary Move Admission

Goal: identify why the `00140.c` call-boundary move record reaches the printer
as `DeferredUnsupported`.

Primary targets: `src/backend/mir/aarch64/codegen/calls.cpp`,
`src/backend/mir/aarch64/codegen/dispatch.cpp`, and the three focused backend
test files

Actions:

- Reproduce or inspect the current `00140.c` failure from `test_after.log`
  without editing expectations or generated artifacts.
- Trace the failing call-boundary move from record construction through
  `call_boundary_move_selection_status` and printer admission.
- Identify the missing or mismatched prepared move fact: source kind/storage,
  destination kind/storage, prepared source register/frame-slot fact, prepared
  destination register fact, op kind, or phase.
- Add or adjust focused backend tests only when they describe the semantic
  selected call-boundary move shape needed for the repair.

Completion check:

- `todo.md` records the localized missing prepared source/destination fact and
  the first implementation target, or records the exact prerequisite owner
  that must be split before this route can proceed.

### Step 2: Repair Prepared Source/Destination Publication

Goal: make supported call-boundary move records carry the structured facts
needed for selected-node admission.

Primary target: AArch64 call-boundary move preparation and value-move bundle
handoff code

Actions:

- Repair the producer or handoff path that loses prepared source/destination
  facts for the localized supported move shape.
- Keep unsupported cases `DeferredUnsupported` with specific diagnostics.
- Do not mark a move selected until the printer can consume both the prepared
  source and prepared destination.
- Keep the change narrow to call-boundary move preparation/selection.

Completion check:

- Focused record or dispatch tests prove the repaired move record is selected
  with printable source and destination data, while incomplete records still
  fail closed.

### Step 3: Prove Machine-Printer Semantics

Goal: prove the selected call-boundary move can be printed by the AArch64
machine printer without weakening printer gates.

Primary target: `backend_aarch64_machine_printer`

Actions:

- Extend printer coverage for the selected call-boundary move shape repaired
  in Step 2.
- Preserve tests that reject non-selected or incomplete call-boundary move
  records.
- Inspect emitted printer output only as evidence of structured operand
  consumption, not as a hard-coded implementation target.

Completion check:

- `backend_aarch64_machine_printer` passes with both positive selected-node
  coverage and fail-closed negative coverage.

### Step 4: Run Focused Proof and Classify Residuals

Goal: prove the focused owner and report any remaining `00140.c` residual
outside the old printer diagnostic.

Actions:

- Run:

```bash
cmake --build build -j && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_target_instruction_records|backend_aarch64_machine_printer|backend_aarch64_instruction_dispatch|c_testsuite_aarch64_backend_src_00140_c)$' > test_after.log 2>&1
```

- Confirm the old `DeferredUnsupported` selected call-boundary move printer
  diagnostic is gone.
- If `00140.c` reaches assembly or runtime and exposes a distinct residual,
  record that residual in `todo.md` without absorbing it into this owner.
- Do not change expectations, unsupported classifications, runners, timeout
  policy, proof-log policy, or CTest registration.

Completion check:

- `test_after.log` contains the focused proof result, `todo.md` records the
  outcome, and any remaining residual is classified separately from selected
  call-boundary move preparation/printing.
