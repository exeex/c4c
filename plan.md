# Calls Printing And Effects Owner Cleanup Runbook

Status: Active
Source Idea: ideas/open/10_calls_printing_and_effects_owner_cleanup.md

## Purpose

Separate call machine-node printing and effect spelling from call semantic
lowering so call-lowering files only own lowering decisions.

## Goal

Printing and effect spelling for calls live under accurate MIR printer,
machine-node, shared-effect, or target-printer owners while preserving existing
assembly, diagnostics, and backend behavior.

## Core Rule

This is ownership cleanup, not semantic call lowering work. Do not change call
argument selection, preservation, dispatch behavior, ABI classification, or test
contracts while moving printer/effect responsibilities.

## Read First

- `ideas/open/10_calls_printing_and_effects_owner_cleanup.md`
- AArch64 call printing and effect helpers under
  `src/backend/mir/aarch64/codegen/`
- shared MIR machine-node and effect printer surfaces under `src/backend/mir`
- printer and backend tests that cover call machine-node output, effects, and
  AArch64 assembly spelling

## Current Targets

- Helpers whose names or locations imply call lowering ownership but only print
  machine nodes, effects, diagnostics, or target-specific spellings.
- Shared MIR effect spelling that can move away from call-lowering files.
- AArch64 target-specific spelling hooks that should remain local but accurately
  named.
- Build metadata, declarations, and includes affected by retired or renamed
  printing/effect files.

## Non-Goals

- Do not change call argument source selection, preservation, republication, or
  call-boundary move semantics.
- Do not rewrite the whole machine printer.
- Do not perform dispatch cleanup.
- Do not consolidate the AArch64 calls file family; that belongs to idea `11`.
- Do not weaken diagnostics, assembly-output coverage, backend expectations, or
  c_testsuite contracts.

## Working Model

- Call lowering owns semantic decisions about calls and target instruction
  emission inputs.
- Shared MIR printers own generic machine-node and effect spelling.
- AArch64 printers or target hooks own AArch64-specific display choices only
  when the spelling is genuinely target-specific.
- A helper that only formats text should not be required by call semantic
  lowering.

## Execution Rules

- Audit before moving: classify each helper as semantic lowering,
  target-specific printing, generic machine printing, or shared effect spelling.
- Keep behavior-preserving moves small enough to prove with printer/backend
  tests after each packet.
- Rename or relocate helpers so the new owner is obvious from the path and
  symbol name.
- Update build metadata and includes in the same packet as any file move,
  deletion, or split.
- Treat semantic changes, output-coverage reductions, and expectation rewrites
  as route failures.

## Ordered Steps

### Step 1: Audit Call Printing And Effect Ownership

Goal: identify which call-family helpers are real lowering logic and which are
printer or effect spelling responsibilities.

Primary targets:

- AArch64 call printing/effect files under
  `src/backend/mir/aarch64/codegen/`
- call helper declarations that expose printer-only APIs
- shared MIR machine-node printers and effect spelling helpers
- existing tests for call machine-node output, effects, diagnostics, and
  AArch64 assembly

Actions:

- List each helper that prints call machine nodes, spells effects, formats
  diagnostics, or provides target-specific display text.
- Classify each helper as semantic call lowering, target-specific printer hook,
  generic machine-node printer, or shared MIR effect spelling.
- Identify dependencies where call lowering includes or calls printer-only
  helpers.
- Record the focused proof set that must stay green for each moved helper.

Completion check:

- The executor can name the helpers to move, their new owners, the helpers that
  must remain target-specific, and the focused tests that prove output
  preservation.

### Step 2: Move Generic Effect Spelling To Shared Owners

Goal: remove generic effect spelling from call-lowering surfaces.

Primary targets:

- shared MIR effect spelling and printer utilities
- call printing/effect helper declarations
- tests that assert effect spelling or diagnostic text

Actions:

- Move generic machine effect spelling into the shared MIR or printer owner.
- Redirect call-family users to the shared owner without changing output.
- Keep target-specific AArch64 effect spelling local only when the spelling
  depends on AArch64 details.
- Delete or rename call-lowering wrappers that only forward to the shared
  spelling helper.

Completion check:

- Generic effects are spelled from a shared owner, call-lowering code no longer
  owns generic effect text, and focused printer/effect tests produce unchanged
  output.

### Step 3: Move Machine-Node Printing Out Of Call Lowering

Goal: relocate call machine-node formatting to printer or machine-node owners
instead of lowering files.

Primary targets:

- call machine-node printing helpers
- shared MIR machine-node printer code
- AArch64 target printer hooks
- build metadata and includes for any renamed or retired files

Actions:

- Move printer-only call machine-node formatting to the appropriate shared MIR
  printer or target-printer file.
- Keep AArch64-only spelling hooks in an AArch64 printer owner with names that
  do not imply semantic lowering authority.
- Remove call-lowering dependencies on printer-only headers or functions.
- Update declarations, includes, and build metadata together with the file
  movement.

Completion check:

- Call semantic lowering no longer depends on printer-only helpers, target
  spelling still has an accurate owner, and existing machine-node output is
  preserved.

### Step 4: Retire Misleading Calls Printing Surfaces

Goal: delete, rename, or split remaining call printing/effect files whose names
still suggest lowering ownership.

Primary targets:

- `calls_printing.cpp` or equivalent call-family printing translation units
- call-family headers that expose display-only APIs
- build metadata and stale includes

Actions:

- Retire `calls_printing.cpp` if all responsibilities have moved, or split and
  rename any target-specific remainder to an accurate printer owner.
- Remove stale includes, declarations, and build entries.
- Keep any remaining call-family helper emission-oriented and free of generic
  display responsibilities.
- Verify the diff does not alter semantic call lowering paths.

Completion check:

- The old misleading calls printing/effect surface is gone or accurately named,
  stale build metadata is removed, and the calls family is smaller without
  behavior change.

### Step 5: Validate Printing And Effects Cleanup

Goal: prove the ownership cleanup preserved behavior and did not hide semantic
call changes in display code.

Primary targets:

- focused printer/effect tests
- AArch64 backend tests that cover call output and diagnostics
- build metadata touched by file moves or deletions
- broader backend or printer subset selected by the supervisor

Actions:

- Run build proof after code-changing packets.
- Run focused printer/effect tests after moving shared effect spelling and
  machine-node formatting.
- Run focused AArch64 backend call-output tests after retiring or renaming call
  printing files.
- Escalate to a supervisor-selected broader backend/printer subset before
  treating the cleanup as complete.

Completion check:

- Build proof and focused printer/backend tests pass, output is preserved, and
  no call semantic change, expectation weakening, or coverage reduction was
  introduced.
