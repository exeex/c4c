# Prepared Scalar `va_arg` Access Plan Runbook

Status: Active
Source Idea: ideas/open/245_prepared_scalar_va_arg_access_plan.md
Activated from: ideas/open/245_prepared_scalar_va_arg_access_plan.md
Parks: ideas/open/243_aarch64_variadic_machine_node_consumption.md

## Purpose

Supply the prepared/shared scalar `va_arg` access-plan fact that idea 243 needs
before AArch64 selected machine-node consumption can resume without local ABI
reconstruction.

## Goal

Expose `helper_operand_homes.va_arg.scalar_access_plan` for representative
supported scalar `va_arg` accesses, with structured source selection,
size/alignment, result-home relationship, and `va_list` progression facts.

## Core Rule

The access plan must be prepared/shared authority. AArch64 target lowering may
consume it later, but this runbook must not implement scalar `va_arg` selected
machine-node consumption or move AAPCS64 access planning into target codegen.

## Read First

- `ideas/open/245_prepared_scalar_va_arg_access_plan.md`
- `ideas/open/243_aarch64_variadic_machine_node_consumption.md`
- `ideas/closed/244_aarch64_variadic_prepared_storage_and_helper_authority.md`
- `src/backend/prealloc/prealloc.hpp`
- `src/backend/prealloc/prealloc.cpp`
- `src/backend/prealloc/prepared_printer.cpp`
- focused prepared and backend tests under `tests/backend/bir/`
- AArch64 fail-closed consumers under `src/backend/mir/aarch64/codegen/`

## Current Targets

- `PreparedVariadicEntryHelperOperandHomes` or an adjacent prepared/shared
  carrier for scalar `va_arg` access planning.
- Preparation logic that already records helper operand homes for scalar
  `va_arg`.
- Prepared-printer output for the new access-plan fact.
- Focused tests that prove the fact exists before selected machine-node
  consumption resumes.

## Non-Goals

- Do not add selected AArch64 scalar `va_arg` machine-node consumption here.
- Do not infer AAPCS64 source selection, register-save offsets, overflow
  offsets, named register counts, or `va_list` progression in AArch64 target
  lowering.
- Do not weaken fail-closed diagnostics or unsupported expectations.
- Do not broaden into aggregate `va_arg`, `va_start`, `va_copy`, or unrelated
  backend machine-node families.

## Working Model

Idea 244 supplied storage, helper scratch, and helper operand-home facts. Idea
243 consumed those facts for `va_start`, then Step 3 stopped with the exact
missing fact `helper_operand_homes.va_arg.scalar_access_plan`. This plan fills
that prepared/shared fact first so the parked consumption runbook can resume
from shared authority rather than target-local reconstruction.

## Execution Rules

- Keep routine packet progress and proof in `todo.md`.
- Add the carrier shape before wiring producers or consumers.
- Prove prepared fact production and printing before touching AArch64 selected
  lowering behavior.
- Treat expectation-only changes, fixture-name matching, diagnostic-only
  rewrites, or target-local ABI reconstruction as route drift.
- For code-changing packets, prove with a build plus the supervisor-chosen
  focused prepared/variadic backend subset.

## Ordered Steps

### Step 1: Define Scalar Access-Plan Carrier

Goal: Add the prepared/shared data shape for scalar `va_arg` access planning.

Actions:

- Inspect the existing helper operand-home carrier and preparation data model.
- Add fields or an adjacent record for scalar source class, size/alignment,
  source storage coordinates, result-home relationship, and `va_list`
  progression.
- Keep the carrier generic across GP register-save, FP register-save, and
  overflow-backed scalar accesses.
- Preserve absence as an explicit incomplete prepared fact.

Completion check:

- The prepared model can represent the scalar access plan without selected
  AArch64 lowering deriving those facts locally.

### Step 2: Populate Prepared Scalar Access Plans

Goal: Produce the access-plan fact for representative supported scalar
`va_arg` helper records.

Actions:

- Wire the preparation path that already identifies scalar `va_arg` helper
  operand homes to fill the access-plan carrier.
- Record GP/FP register-save versus overflow source selection from prepared
  ABI facts, not from target-codegen reconstruction.
- Record per-access size/alignment and post-access `va_list` progression.
- Leave unsupported or incomplete cases fail-closed with the exact missing
  fact.

Completion check:

- Focused preparation tests can observe complete and incomplete scalar
  access-plan facts.

### Step 3: Print And Diagnose The Prepared Fact

Goal: Make the scalar access-plan fact visible enough for tests and downstream
consumers.

Actions:

- Extend prepared-printer output for the scalar access plan.
- Preserve clear diagnostics when the fact is missing or incomplete.
- Avoid claiming selected machine-node support through prepared dump text.

Completion check:

- Prepared-printer tests show the scalar access plan, and existing fail-closed
  diagnostics still name `helper_operand_homes.va_arg.scalar_access_plan` when
  the fact is absent.

### Step 4: Validate And Hand Back To Idea 243

Goal: Prove the prerequisite and leave a clean handoff for scalar `va_arg`
machine-node consumption.

Actions:

- Run the supervisor-chosen build and focused prepared/variadic backend subset.
- Summarize supported scalar access-plan cases, unsupported cases, and any
  remaining prepared fact gaps in `todo.md`.
- Ask the supervisor to route closure or reactivation of idea 243 once the
  prerequisite is proven.

Completion check:

- The prerequisite fact is proven by focused tests, no selected machine-node
  support is claimed here, and idea 243 has a clear resume point.
