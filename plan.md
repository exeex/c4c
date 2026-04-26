# BIR Printer Structured Render Context Runbook

Status: Active
Source Idea: ideas/open/117_bir_printer_structured_render_context.md

## Purpose

Give BIR dump rendering an explicit structured render path so
`src/backend/bir/bir_printer.cpp` can print spellings from structured
authority instead of treating raw legacy type strings as the long-term source.

## Goal

Make selected BIR printer paths structured-first while preserving existing
`--dump-bir` text and keeping legacy string fallbacks available until idea 118.

## Core Rule

Preserve existing BIR dump output unless a printer contract change is
intentional, documented in this runbook or `todo.md`, and covered by tests.

## Read First

- ideas/open/117_bir_printer_structured_render_context.md
- src/backend/bir/bir_printer.cpp
- src/backend/bir/
- src/backend/lir_to_bir.cpp
- src/backend/lir.hpp
- src/backend/layout.cpp
- tests that exercise `--dump-bir` and prepared BIR output

## Current Targets

- Minimal BIR render context or `bir::Module` extension for structured
  spelling state.
- Struct/type spelling state carried from LIR-to-BIR lowering into BIR dump
  rendering.
- Call return type rendering, especially paths currently depending on
  `bir::CallInst::return_type_name`.
- Stability of function, global, local slot, block, and string constant
  spellings.
- Printer tests comparing legacy-backed and structured-backed output for
  aggregate-sensitive paths.
- Inventory of any raw-string printer fallbacks left for idea 118.

## Non-Goals

- Do not remove legacy `type_decls` or fallback helpers.
- Do not broadly redesign `LirTypeRef` identity.
- Do not migrate MIR.
- Do not change prepared BIR printer contracts except where they embed
  semantic BIR output from `bir::print`.
- Do not make `bir_printer.cpp` parse legacy type declaration bodies.
- Do not rewrite expectations to claim progress without structured authority.

## Working Model

- `bir::Module` currently contains enough scalar facts for many dump paths but
  does not carry a general structured type spelling context.
- Raw string fields can remain compatibility fallbacks during this runbook.
- The printer should prefer explicit structured spelling data when it is
  available, and fall back only where coverage has not yet been migrated.
- Idea 118 owns removing fallback-only legacy type text after this structured
  render route is proven.

## Execution Rules

- Prefer a small explicit render context over making the printer reach back
  into LIR internals.
- Keep changes behavior-preserving and prove dump text stability after each
  code-changing step.
- Record any fallback-only string field in `todo.md` as a removal candidate
  for idea 118.
- Keep structured spelling coverage semantic and reusable; do not add
  testcase-shaped rendering shortcuts.
- Use `test_after.log` for executor proof unless the supervisor delegates a
  different artifact.

## Steps

### Step 1: Inventory Printer Spelling Authority

Goal: Identify every BIR dump spelling path that already has structured
authority, uses final spelling data, or depends on legacy/raw type text.

Primary target: `src/backend/bir/bir_printer.cpp` and BIR data structures.

Actions:
- Inspect BIR printer entry points and helper functions.
- Classify printed names and types as structured-ready, final spelling data,
  legacy fallback, or unresolved.
- Pay special attention to `bir::CallInst::return_type_name` and aggregate
  type-sensitive dump output.
- Record the first implementation target and remaining fallback-only surfaces
  in `todo.md`.

Completion check:
- `todo.md` names the first code-changing target and lists raw-string printer
  fallbacks that must either be covered here or deferred to idea 118.

### Step 2: Define Minimal Structured Render Context

Goal: Add the smallest BIR-facing render context needed for printer spelling
without pulling LIR internals into `bir_printer.cpp`.

Primary target: BIR module/rendering types and LIR-to-BIR handoff code.

Actions:
- Decide whether the structured context belongs as a dedicated render context
  object or as a narrow `bir::Module` extension.
- Carry struct/type spelling state needed by covered dump paths.
- Preserve existing final spelling fields for function, global, local, block,
  and string constant names.
- Keep legacy strings available as fallback and compatibility evidence.

Completion check:
- The BIR printer can receive structured spelling state through an explicit
  BIR-facing API, and existing build/tests still pass for unchanged output.

### Step 3: Make Call Return Type Rendering Structured-First

Goal: Move call return type dump rendering away from raw
`CallInst::return_type_name` as active authority for covered paths.

Primary target: `bir::CallInst` construction and BIR printer call rendering.

Actions:
- Add or use structured type identity/spelling data for call return types.
- Render through the structured context when available.
- Keep `return_type_name` only as compatibility fallback where needed.
- Add focused dump coverage for aggregate-sensitive call return spelling.

Completion check:
- Covered call return dump output is byte-stable while exercising the
  structured-first route.

### Step 4: Cover Aggregate-Sensitive Printer Paths

Goal: Prove structured-backed BIR dump output matches legacy-backed spelling
for selected aggregate-sensitive cases.

Primary target: CLI/backend dump tests around aggregate BIR output.

Actions:
- Add tests that compare or otherwise prove stable output for representative
  aggregate-sensitive paths.
- Include a structured-only or legacy-shadow-absent fixture where practical.
- Keep fallback behavior explicit for paths that cannot yet be migrated.
- Avoid changing dump contracts outside the covered semantic path.

Completion check:
- New or updated BIR printer tests pass and demonstrate structured-backed
  rendering for the selected aggregate-sensitive output.

### Step 5: Preserve Prepared BIR And Existing Dump Contracts

Goal: Ensure the render context change does not break prepared BIR workflows
or existing dump filters.

Primary target: prepared BIR printer tests and CLI dump tests.

Actions:
- Run focused prepared-BIR and `--dump-bir` tests selected by the supervisor.
- Fix API seams that make prepared BIR construction awkward without adding
  broad compatibility rewrites.
- Document any intentional output contract change in `todo.md` before broader
  validation.

Completion check:
- Existing prepared-BIR and CLI dump expectations remain stable, or any
  intentional change is explicitly documented and tested.

### Step 6: Inventory Remaining Fallbacks For Idea 118

Goal: Leave idea 118 a precise list of legacy printer fallbacks that are now
removable or still blocked.

Primary target: `todo.md`, BIR printer fallback sites, and focused proof logs.

Actions:
- Enumerate raw string fields that remain printer fallbacks.
- Mark which covered paths no longer use legacy strings as active authority.
- Run the delegated narrow proof and any supervisor-selected broader
  validation after multiple printer paths have changed.
- Do not remove legacy fields in this runbook unless the source idea already
  makes that field fallback-only and removal is explicitly delegated.

Completion check:
- `todo.md` contains a clear handoff inventory for idea 118, and focused proof
  shows structured-first rendering for the covered paths.
