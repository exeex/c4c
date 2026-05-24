# Shared Prepared Consumer Missing-Fact Diagnostics

Status: Active
Source Idea: ideas/open/prepared-consumer-missing-fact-diagnostics.md

## Purpose

Create shared diagnostic builders for missing Prepared facts currently reported
from AArch64 dispatch.

Goal: extract one small target-independent Prepared-consumer diagnostic surface
that x86 and later RISC-V can reuse without weakening messages or hiding
supported-path failures.

## Core Rule

Shared diagnostics may describe missing Prepared authority or machine-consumer
context; targets still own target-specific ABI, instruction, operand, and
emission errors.

## Read First

- `ideas/open/prepared-consumer-missing-fact-diagnostics.md`
- `src/backend/mir/aarch64/codegen/dispatch_diagnostics.cpp`
- `src/backend/mir/aarch64/codegen/dispatch.hpp`
- `src/backend/mir/aarch64/codegen/dispatch_lookup.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_publication.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_producers.cpp`
- `src/backend/mir/aarch64/codegen/operands.cpp`
- `src/backend/prealloc/prepared_lookups.hpp`
- `src/backend/prealloc/prepared_lookups.cpp`
- `src/backend/mir/x86/prepared/prepared.hpp`
- `src/backend/mir/x86/prepared/dispatch.cpp`

## Current Targets

- AArch64 missing Prepared-fact diagnostic builders and call sites, especially
  missing function context, value authority, typed register authority, storage
  plan, call plan, block mapping, and instruction mapping.
- Prealloc fact-owner-facing diagnostic helpers when the missing authority is a
  Prepared fact.
- Shared MIR support only when the diagnostic is about consumer stream or
  machine-record context rather than fact ownership.
- x86 prepared consumers that can reuse the same missing authority categories
  without copying AArch64 message construction.

## Non-Goals

- Do not rewrite all backend diagnostics.
- Do not move target-specific AArch64 instruction, ABI, register conversion, or
  operand legality errors into shared Prepared diagnostics.
- Do not change failure classification to hide supported compiler paths or
  reclassify them as unsupported.
- Do not combine diagnostics extraction with functional lowering changes.
- Do not weaken diagnostics into generic `lowering failed` messages.

## Working Model

- Diagnostics tied to missing Prepared authority should live near the owner of
  those facts, usually `src/backend/prealloc`.
- Diagnostics tied to machine-consumer context may belong in shared MIR only if
  the context is target-independent.
- AArch64 should consume shared builders for target-independent categories while
  keeping target emission errors local.
- x86 should be able to call the same builders for prepared-consumer failures.

## Execution Rules

- Start with inventory before adding helper APIs.
- Extract one small diagnostic-builder surface first; do not build a broad
  diagnostics framework.
- Preserve or improve diagnostic specificity.
- Keep each implementation step behavior-preserving unless the source idea
  explicitly requires a diagnostic boundary change.
- Treat generic message weakening, unsupported reclassification, named-testcase
  matching, or message-only capability claims as route failure.
- After code-changing steps, run a fresh build or compile proof plus the
  delegated narrow test subset.
- Before final acceptance, prove more than one missing-fact category and show
  x86 reuse where feasible.

## Ordered Steps

### Step 1: Inventory Missing Prepared-Fact Diagnostics

Goal: identify target-independent missing Prepared-fact diagnostic categories
currently owned by AArch64 and separate them from target-specific emission
errors.

Primary targets:

- `src/backend/mir/aarch64/codegen/dispatch_diagnostics.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_lookup.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_publication.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_producers.cpp`
- `src/backend/mir/aarch64/codegen/operands.cpp`
- `src/backend/prealloc/prepared_lookups.hpp`

Actions:

- Inventory AArch64 diagnostic categories and messages for missing function
  context, value authority, typed register authority, storage plan, call plan,
  block mapping, instruction mapping, and adjacent Prepared-consumer failures.
- Classify each category as prealloc fact-owner-facing, shared MIR
  consumer-context-facing, or target-specific AArch64 emission error.
- Choose one narrow first extraction surface with at least two
  target-independent missing-fact categories.
- Record the chosen categories, destination rationale, and target-local
  diagnostic categories in `todo.md`.

Completion check:

- `todo.md` names the selected diagnostic categories, destination layer, and
  categories that must remain target-local.

### Step 2: Add Shared Prepared Diagnostic Builders

Goal: introduce a small shared diagnostic-builder API for the selected
target-independent missing Prepared-fact categories.

Primary targets:

- `src/backend/prealloc`
- `src/backend/prealloc/prepared_lookups.hpp`
- `src/backend/prealloc/prepared_lookups.cpp`
- Shared MIR support only if Step 1 chooses consumer context instead of fact
  ownership.
- Focused tests under `tests/backend` if a direct harness can exercise the
  builders.

Actions:

- Define builder functions or records for the selected missing-fact categories.
- Preserve category names and messages specific enough to identify the missing
  Prepared authority.
- Avoid dependencies on AArch64 target types.
- Add focused direct coverage if available.
- Run a build or compile proof for the new API surface.

Completion check:

- Shared builders exist for the selected categories and do not weaken message
  specificity or depend on AArch64 instruction/ABI types.

### Step 3: Adapt AArch64 To Use Shared Builders

Goal: make AArch64 prepared-consumer diagnostics use the shared builders while
keeping target-specific diagnostics local.

Primary targets:

- `src/backend/mir/aarch64/codegen/dispatch_diagnostics.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_lookup.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_publication.cpp`
- `src/backend/mir/aarch64/codegen/operands.cpp`
- Existing AArch64 diagnostic or prepared-lowering tests under `tests/backend`

Actions:

- Replace selected target-independent message construction with calls to the
  shared builders.
- Keep AArch64 instruction, ABI, operand legality, register conversion, and
  emission diagnostics local.
- Preserve existing failure status and message specificity.
- Prove before/after diagnostics for more than one selected category.

Completion check:

- AArch64 uses shared builders for the selected missing Prepared-fact
  categories without weakening or reclassifying diagnostics.

### Step 4: Prove Reuse Path For x86 Prepared Consumers

Goal: document or wire a concrete x86 prepared-consumer path for the shared
diagnostic builders.

Primary targets:

- `src/backend/mir/x86/prepared/prepared.hpp`
- `src/backend/mir/x86/prepared/dispatch.cpp`
- `src/backend/mir/x86/debug/prepared_route_debug.cpp.md`
- Existing x86 prepared tests under `tests/backend/bir`

Actions:

- Identify the x86 prepared consumer path that should use the shared builders.
- Add a narrow call, type use, adapter, or nearby code note showing x86 reuse.
- Keep x86 target-specific emission and ABI errors local.
- Add or update focused tests if reuse can be proved without a full x86
  lowering rewrite.

Completion check:

- There is concrete code or test evidence that x86 can report the same missing
  Prepared authority categories without duplicating AArch64 message builders.

### Step 5: Validate Diagnostics And Anti-Overfit Coverage

Goal: prove the diagnostic extraction did not weaken supported-path failures or
claim functional progress from message-only changes.

Actions:

- Run the supervisor-delegated build or compile proof.
- Run narrow backend/prepared tests covering more than one missing-fact
  category.
- Include an x86 reuse proof if Step 4 added a query or adapter.
- Check that messages remain specific enough to identify the missing Prepared
  authority.
- Confirm no tests are weakened, no supported path is reclassified as
  unsupported, and no functional lowering progress is claimed from diagnostics
  alone.

Completion check:

- `test_after.log` or the delegated proof artifact records passing validation
  for multiple diagnostic categories.
- `todo.md` records the x86 reuse evidence and anti-overfit result.

## Final Acceptance

- Shared diagnostic builders exist for a small set of target-independent
  missing Prepared-fact categories.
- AArch64 uses the shared builders while target-specific diagnostics remain
  local.
- x86 has a concrete reuse path for the shared builders.
- Proof covers more than one missing-fact category without weakening messages,
  reclassifying supported paths, or claiming functional lowering progress from
  message-only changes.
