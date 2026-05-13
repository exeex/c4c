# AArch64 Prepared-Module MIR Boundary Runbook

Status: Active
Source Idea: ideas/open/204_aarch64_prepared_module_mir_boundary.md

## Purpose

Create the first small code-oriented AArch64 follow-up after the markdown-first
backend reconstruction: a target-local MIR boundary that consumes
`c4c::backend::prepare::PreparedBirModule`.

## Goal

Introduce a compile-proven AArch64-owned MIR handoff with structured-id keyed
module, function, block, operand, register, frame, branch, call, move, and data
side-table skeletons, without starting instruction selection or assembly text
emission.

## Core Rule

The AArch64 boundary must preserve semantic identity from prepared-module data
and structured prepared ids. Do not recover facts from rendered names, printed
BIR, legacy LIR text, assembly strings, parser operands, or markdown examples.

## Read First

- `ideas/open/204_aarch64_prepared_module_mir_boundary.md`
- `src/backend/mir/aarch64/BIR_PREPARED_GAP_LEDGER.md`
- `src/backend/mir/aarch64/BACKEND_ENTRY_CONTRACT.md`
- `src/backend/prealloc/prealloc.hpp`
- `src/backend/mir/x86/api/api.hpp`
- `src/backend/mir/x86/module/module.hpp`
- `src/backend/mir/x86/prepared/prepared.hpp`
- `tests/backend/CMakeLists.txt`

## Current Targets

- New or extended AArch64 target-local MIR surface under
  `src/backend/mir/aarch64/`.
- Public handoff API accepting `const c4c::backend::prepare::PreparedBirModule&`.
- Focused backend tests under `tests/backend/`.
- Build wiring in `src/backend/CMakeLists.txt` and `tests/backend/CMakeLists.txt`
  only as needed for the new boundary and tests.

## Non-Goals

- Do not add AArch64 instruction selection.
- Do not add or route through assembly text emission.
- Do not add an assembler, object writer, linker, or executable production.
- Do not accept raw `bir::Module` as the AArch64 target-local lowering input.
- Do not implement memory lowering that depends on volatility or non-default
  address-space facts not yet carried by shared preparation.
- Do not broaden into NEON, broad vector support, inline assembly, f128/i128
  special lowering, or target ABI completion beyond fields required by this MIR
  skeleton.

## Working Model

- Treat `PreparedBirModule` as the only semantic input to the AArch64 boundary.
- Gate construction on an AArch64 `TargetProfile` and AAPCS64 backend ABI before
  building target MIR records.
- Keep target registers distinct from semantic value identity.
- Preserve prepared identities in target-local skeleton records even when later
  lowering fields are intentionally empty.
- Reserve documented placeholder fields for later target MIR, target ABI,
  instruction-selection, assembler/object, or shared-preparation work.

## Execution Rules

- Keep each implementation step compile-proven before moving to the next.
- Prefer small tests that inspect target-local records and rejection behavior
  directly.
- If a step uncovers a missing shared prepared carrier, record that in
  `todo.md`; create a separate idea only if the missing carrier is required to
  complete this boundary.
- Reject expectation weakening, unsupported reclassification, and named-case
  shortcuts as progress.
- Preserve unrelated dirty files and transient `review/` artifacts.

## Steps

### Step 1: Inspect Prepared/AArch64 Boundary Surfaces

Goal: identify the minimal existing APIs and build targets needed for a
prepared-module handoff.

Primary target: `src/backend/mir/aarch64/`, `src/backend/prealloc/`, and nearby
x86 prepared-module handoff patterns.

Actions:

- Inspect `PreparedBirModule` fields and available target-profile/ABI metadata.
- Inspect existing x86 prepared handoff APIs only for shape and build wiring;
  do not copy x86 lowering behavior.
- Decide the file layout for the AArch64 MIR skeleton and handoff entry.
- Identify the narrow test command the executor should run for the first slice.

Completion check:

- `todo.md` names the chosen implementation files, first test target, and exact
  proof command for Step 2.

### Step 2: Add Prepared-Module Handoff Gate

Goal: add the AArch64 entry surface that accepts `PreparedBirModule` and rejects
unsupported target profiles or non-AAPCS64 ABI before MIR construction.

Primary target: AArch64 boundary API under `src/backend/mir/aarch64/`.

Actions:

- Add a target-local handoff function or builder that takes
  `const c4c::backend::prepare::PreparedBirModule&`.
- Resolve and validate the prepared module's `TargetProfile`.
- Require the AArch64 route and AAPCS64 ABI before target MIR construction.
- Return a typed success/error result or use the repo's established diagnostic
  pattern for backend handoff failures.
- Add focused tests for accepted AArch64/AAPCS64 input and rejected non-AArch64
  or wrong-ABI input.

Completion check:

- The new handoff compiles and focused tests prove gate behavior without
  constructing instruction-selection or assembly output.

### Step 3: Define Target-Local MIR Module, Function, And Block Records

Goal: introduce the smallest AArch64-owned MIR container records keyed by
structured prepared ids.

Primary target: AArch64 MIR skeleton headers/sources under
`src/backend/mir/aarch64/`.

Actions:

- Define module, function, and block records that retain prepared module,
  function-name, function, and block-label identities where available.
- Populate records from prepared control-flow/module facts after the handoff
  gate succeeds.
- Keep display strings as debug-only or optional labels, not semantic keys.
- Add tests for representative function and block identity preservation.

Completion check:

- Tests can inspect an AArch64 MIR module built from a prepared module and verify
  structured function/block identity preservation.

### Step 4: Add Operand And Register Skeletons

Goal: preserve prepared value identity and distinguish semantic values from
target register references.

Primary target: AArch64 operand/register records.

Actions:

- Define operand records for prepared value ids, value names, types, symbols,
  strings, and frame-slot identities where present.
- Define target register class and physical-register reference records without
  conflating them with semantic values.
- Populate representative operands from prepared value-location, regalloc, and
  storage-plan facts that already exist.
- Document any fields intentionally left empty for later target ABI or
  instruction-selection work.

Completion check:

- Focused tests prove representative prepared value and target register
  identities survive the handoff as structured fields.

### Step 5: Add Frame, Branch, Call, And Move Skeletons

Goal: carry prepared control, frame, stack, call, and movement facts into
target-local records without lowering them to instructions.

Primary target: AArch64 frame/control/call/move records.

Actions:

- Add frame, stack-slot, dynamic-stack, and callee-save records sourced from
  prepared frame and stack plans.
- Add branch and compare records sourced from prepared control-flow facts.
- Add call records sourced from prepared call plans.
- Add move, copy, spill, reload, ABI-binding, and parallel-copy records sourced
  from prepared value-location, regalloc, storage-plan, and parallel-copy facts.
- Keep all records descriptive; do not select concrete AArch64 instructions.

Completion check:

- Focused tests verify representative frame, branch, call, and move records
  preserve the relevant prepared ids and remain instruction-free.

### Step 6: Add Data/Object Side-Table Skeletons

Goal: preserve module-level data/object facts needed by later assembler/object
work without starting that work.

Primary target: AArch64 data side-table records.

Actions:

- Add records for globals, strings, symbol visibility, TLS, constants,
  initializers, and later relocation needs when those facts are present in the
  prepared module.
- Keep relocation/object-emission fields documented as placeholders.
- Do not emit assembly text, object sections, relocations, or binary output.

Completion check:

- Focused tests verify representative data identities and visibility facts are
  retained as structured fields only.

### Step 7: Consolidate Boundary Proof

Goal: prove the boundary is complete for this idea and ready for reviewer
scrutiny.

Primary target: build/test coverage and documentation comments for deferred
fields.

Actions:

- Run a fresh build or compile proof for the touched targets.
- Run the focused AArch64 MIR boundary tests and relevant backend prepare tests.
- Escalate to a broader backend CTest subset if the implementation touches
  shared prepare or backend public routing.
- Confirm no instruction selection, assembly text emission, assembler/object,
  linker, rendered-name recovery, or expectation weakening entered the slice.
- Update `todo.md` with final proof commands and any deferred-field notes.

Completion check:

- `todo.md` records green proof for the boundary and any remaining deferred work
  is explicitly categorized as later target MIR, target ABI,
  instruction-selection, assembler/object, or shared-preparation scope.
