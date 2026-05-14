# AArch64 Variadic Function Entry Carriers Runbook

Status: Active
Source Idea: ideas/open/232_aarch64_variadic_function_entry_carriers.md
Activated from: ideas/open/232_aarch64_variadic_function_entry_carriers.md

## Purpose

Represent full AAPCS64 variadic function-entry facts as prepared/shared
carriers before AArch64 machine-node lowering consumes them.

## Goal

Expose structured prepared facts for variadic callee entry, register-save
areas, `va_list` layout, named GP/FP argument counts, overflow-area state, and
helper-call resource needs without rebuilding those ABI decisions in AArch64
target codegen.

## Core Rule

Prepared/shared layers own variadic entry authority. AArch64 lowering and the
printer may consume explicit facts, but must not infer `va_list` layout,
register-save slots, negative offsets, named-argument counts, overflow-area
bases, or helper scratch policy from rendered text, archived markdown, or
local ABI reconstruction.

## Read First

- `ideas/open/232_aarch64_variadic_function_entry_carriers.md`
- `src/backend/prealloc/prealloc.hpp`
- `src/backend/prealloc/prealloc.cpp`
- `src/backend/prealloc/prepared_printer.cpp`
- BIR variadic call and `va_start` / `va_arg` lowering surfaces
- AArch64 lowering diagnostics and selected machine-node entry points that
  currently consume `PreparedCallPlan` and `PreparedFramePlanFunction`

## Current Targets

- Prepared variadic function-entry metadata distinct from variadic call-boundary
  metadata.
- Explicit named GP/FP argument counts for AArch64 variadic callees.
- Explicit register-save-area, overflow-area, and `va_list` layout carriers.
- Prepared dump coverage that makes those facts observable.
- Fail-closed AArch64 consumption points that do not recompute missing facts.

## Non-Goals

- Do not implement final `va_start`, `va_arg`, `va_copy`, or full machine-node
  lowering in this plan unless the prepared carriers are already explicit and
  the supervisor narrows a later packet to consumption only.
- Do not create register-save areas, spill slots, stack offsets, or helper
  scratch policy inside AArch64 target codegen.
- Do not treat `PreparedCallWrapperKind::DirectExternVariadic` or
  `PreparedCallPlan::variadic_fpr_arg_register_count` as sufficient evidence
  for full variadic function-entry support.
- Do not weaken variadic tests, mark supported paths unsupported, or add
  named-case shortcuts.
- Do not absorb global address, memory load/store, scalar cast, i128,
  binary128, atomic, intrinsic, inline-asm, callee-save slot placement, or
  preserved-value extent work.

## Working Model

The existing prepared call plan covers call-boundary variadic metadata. This
plan adds a separate prepared function-entry model for the callee side:

1. identify variadic callee entry requirements from BIR/function metadata;
2. record the ABI-derived entry facts in shared prepared structures;
3. expose those facts through prepared dumps and focused tests;
4. leave AArch64 machine-node lowering blocked or diagnostic-only until it can
   consume complete structured facts.

## Execution Rules

- Keep routine packet progress and proof in `todo.md`.
- Preserve source intent in the idea file; use `plan.md` only for execution
  boundaries and `todo.md` for packet state.
- Prefer small packets: inspect current facts, add carrier types, populate
  minimal metadata, expose dumps, then add fail-closed consumers.
- Treat expectation-only changes, unsupported downgrades, text-only printer
  patches, or case-name matching as route drift.
- When a missing prepared fact blocks real lowering, record the blocker in
  `todo.md` rather than inventing AArch64-local state.
- For code-changing packets, prove with a build plus the supervisor-chosen
  prepared/BIR variadic subset; escalate to backend or AArch64 subsets when
  shared prepared structures affect broader lowering.

## Ordered Steps

### Step 1: Inspect Variadic Entry Inputs And Existing Prepared Boundaries

Goal: Identify where the compiler currently knows a function is variadic, where
`va_start` / `va_arg` / `va_copy` appear in BIR, and where prepared call/frame
metadata is produced and dumped.

Primary targets:

- `PreparedCallPlan` and `PreparedFramePlanFunction`
- BIR function metadata and variadic intrinsic calls
- prepared printer / dump output
- AArch64 diagnostics that currently reject incomplete prepared facts

Actions:

- Inspect how variadic callees, named parameters, call-boundary variadic counts,
  and `va_start` / `va_arg` BIR calls are represented today.
- Identify the prepared structure that should own variadic function-entry facts.
- Identify representative narrow proof cases for integer, FP, aggregate, and
  `va_copy` entry metadata.
- Record any missing semantic inputs that must be fixed outside AArch64 codegen.

Completion check:

- `todo.md` records the concrete carrier insertion points, prepared dump
  surfaces, initial proof subset, and any missing inputs that must not be
  fabricated in target lowering.

### Step 2: Define Prepared Variadic Entry Carrier Types

Goal: Add typed shared/prepared records for AArch64 variadic function-entry
metadata without wiring them into target codegen yet.

Actions:

- Add carrier fields for named GP and FP argument counts.
- Add register-save-area facts, including GP/FP save ranges or slots and
  negative offset policy as structured data.
- Add overflow-area base and `va_list` field-layout facts as explicit records.
- Add helper-call resource needs only as prepared facts, not as target-local
  scratch choices.
- Keep the call-boundary variadic count separate from the new function-entry
  model.

Completion check:

- Prepared data structures can represent the selected proof cases without
  string payloads, markdown-era conventions, or AArch64-local ABI decisions.

### Step 3: Populate Variadic Entry Facts In Prepared State

Goal: Build the new carriers from existing semantic/BIR/prepared inputs before
machine lowering runs.

Actions:

- Detect AArch64 variadic callees and named parameter counts from shared
  function metadata.
- Populate register-save-area and `va_list` layout facts from ABI policy in the
  prepared/shared layer.
- Link `va_start`, `va_arg`, and `va_copy` helper needs to the prepared entry
  facts where the current IR exposes them.
- Fail explicitly when required semantic inputs are missing.

Completion check:

- Prepared state for focused variadic callees contains complete structured
  entry facts, and non-variadic functions do not receive misleading entry
  metadata.

### Step 4: Expose Prepared Dumps And Focused Tests

Goal: Make variadic entry carriers observable before downstream AArch64
machine-node consumption.

Actions:

- Extend prepared dump output for the new carrier family.
- Add or update focused backend/BIR tests that require the new dump snippets for
  variadic integer, FP, aggregate, and `va_copy` cases.
- Keep existing call-boundary variadic tests distinct from function-entry
  carrier tests.

Completion check:

- Dump-based proof shows explicit variadic function-entry facts for the focused
  cases, and no test relies on final machine-node lowering.

### Step 5: Add Fail-Closed AArch64 Consumption Guards

Goal: Prevent downstream AArch64 lowering from silently recomputing variadic
entry facts while keeping final lowering deferred.

Actions:

- Teach AArch64 entry or variadic-intrinsic lowering surfaces to require the
  new prepared carriers before attempting any variadic entry behavior.
- Emit explicit diagnostics for missing or incomplete variadic entry facts.
- Do not print register saves, `va_list` setup, `va_arg`, or `va_copy` machine
  output unless all structured facts are present and the supervisor delegates
  that consumption slice.

Completion check:

- AArch64 variadic entry paths are either blocked by clear prepared-fact
  diagnostics or consume explicit carriers; they do not reconstruct ABI state
  locally.

### Step 6: Validate And Summarize

Goal: Prove the prepared-carrier route and preserve remaining downstream work
at the correct lifecycle layer.

Actions:

- Run the supervisor-chosen build and narrow variadic prepared/BIR proof subset.
- Escalate to backend or AArch64 subsets if the changed prepared structures
  affect shared lowering or dumps.
- Record remaining unsupported `va_start`, `va_arg`, `va_copy`, and machine-node
  consumption states in `todo.md`.
- Ask the supervisor whether this source idea is complete, blocked, or should
  remain open pending downstream AArch64 machine-node work.

Completion check:

- The selected variadic callee cases expose structured prepared entry facts, and
  downstream unsupported states are explicit non-overfit follow-up blockers.
