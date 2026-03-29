# x86 Backend Bring-Up Runbook

Status: Active
Source Idea: ideas/open/15_backend_x86_port_plan.md

## Purpose

Turn the already translated `src/backend/x86/` mirror from passive files into an active backend path in small compileable slices.

## Goal

Make one minimal x86 codegen slice build-participating, reconnect it to the backend driver through an explicit x86 adapter seam, and emit assembly for a trivial integer-return function.

## Core Rule

Preserve ref-shaped structure and keep the x86 seam narrow. Do not broaden this plan into assembler parity, linker parity, or general backend cleanup.

## Read First

- `ideas/open/15_backend_x86_port_plan.md`
- `ref/claudes-c-compiler/src/backend/x86/mod.rs`
- `ref/claudes-c-compiler/src/backend/x86/codegen/`
- `src/backend/x86/`
- existing backend driver and LIR adapter surfaces under `src/backend/`

## Current Scope

- stabilize the mirrored `src/backend/x86/` tree enough that key entry files are include-clean
- reconnect `src/backend/x86/mod.cpp` and `src/backend/x86/codegen/mod.cpp` to the current backend entry surface
- add the smallest x86-specific adapter needed between existing LIR structures and the ref-shaped x86 codegen
- prove one minimal vertical slice:
  integer return,
  prologue/epilogue,
  direct return path,
  and emitted x86 assembly text for a trivial function

## Non-Goals

- full x86 assembler integration
- full x86 linker integration
- broad optimization or abstraction cleanup
- unrelated AArch64 or shared-backend redesign
- broad behavioral coverage beyond the first supported slice

## Working Model

Phase 1 preserves the mirror and makes top-level files structurally sound.

Phase 2 reconnects the mirror in thin vertical slices:

- backend driver entry
- x86 adapter seam
- minimal codegen path
- narrow validation

Treat compile integration as monotonic progress. It is acceptable for the whole x86 tree to remain incomplete as long as the active slice is explicit and build-participating.

## Execution Rules

- prefer mechanical translation fixes over design rewrites
- keep file layout close to `ref/claudes-c-compiler/src/backend/x86/`
- if a new requirement is needed to finish the current slice, keep it minimal and local
- if adjacent work appears that is not required for the current slice, record it back in `ideas/open/15_backend_x86_port_plan.md` instead of expanding this runbook
- add or update focused tests before changing behavior
- validate against current backend expectations and reference behavior where practical

## Ordered Steps

### Step 1: Audit And Normalize The Mirrored x86 Tree

Goal: confirm the translated `src/backend/x86/` mirror still matches the intended ref coverage and identify missing or structurally broken entry files.

Primary targets:

- `src/backend/x86/`
- `ref/claudes-c-compiler/src/backend/x86/`

Actions:

- inspect the mirrored x86 tree against the ref x86 module layout
- fill obvious file-coverage gaps only where missing entry files block the first compile slice
- normalize includes, namespaces, and forward declarations in top-level x86 files without broad semantic rewrites

Completion check:

- the x86 mirror coverage and top-level file set are explicit
- missing files or major mismatches that block the first slice are either fixed or recorded in `plan_todo.md`

### Step 2: Make Top-Level x86 Entry Surfaces Include-Clean

Goal: make `mod.cpp` and `codegen/mod.cpp` structurally compile-ready even if deeper files remain stubbed.

Primary targets:

- `src/backend/x86/mod.cpp`
- `src/backend/x86/codegen/mod.cpp`

Actions:

- fix include graphs, declarations, and obvious translation seams that prevent these files from participating in the build
- keep temporary placeholders explicit where deeper functionality is not yet wired
- avoid pulling assembler or linker implementation into scope unless required for the minimal codegen slice

Completion check:

- top-level x86 entry surfaces parse and compile in the active build configuration, or the remaining blockers are localized and documented

### Step 3: Reconnect The Backend Driver Through A Narrow x86 Adapter

Goal: make the backend driver able to instantiate the x86 path through a target-local seam rather than placeholder or unreachable logic.

Primary targets:

- current backend driver entry surfaces under `src/backend/`
- x86 target-local adapter surfaces under `src/backend/x86/`

Actions:

- identify the narrowest backend-owned entry point required for x86
- add explicit x86 adapter code between current LIR/backend structures and ref-shaped x86 codegen expectations
- keep shared backend changes minimal and targeted to the x86 seam

Completion check:

- the backend driver can select the x86 path through an explicit backend-owned entry surface
- the x86 adapter seam is local and understandable

### Step 4: Land The First Minimal Emit Slice

Goal: emit backend-owned x86 assembly text for a trivial integer-return function.

Primary targets:

- minimal x86 codegen emit path
- focused x86 validation case

Actions:

- port only the minimum codegen path needed for function prologue/epilogue and integer return
- support the direct return path for a trivial function
- add or update a focused test that proves the x86 backend path emits assembly text through the mirrored codegen path

Completion check:

- a trivial integer-return case reaches the x86 backend path and emits assembly text
- the validating test passes

### Step 5: Record Remaining Gaps For The Next Slice

Goal: leave the next execution slice obvious without expanding scope.

Actions:

- capture what remains for ALU, compare/branch, calls, stack locals, and RIP-relative globals
- record blockers, deferred assembler/linker work, and any follow-on ideas in `plan_todo.md` or the source idea as appropriate

Completion check:

- the next thin vertical slice is explicitly named and resumable

## Validation

- build the active slice in the normal build
- add or update focused tests before implementation
- confirm the backend driver reaches the x86 path instead of unrelated fallback logic
- for the first supported slice, verify emitted assembly text for a trivial integer-return function
- run broader regression checks before handoff once the slice is implemented
