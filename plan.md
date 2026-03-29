# Backend AArch64 Global Int Pointer Roundtrip Runbook

Status: Active
Source Idea: ideas/open/14_backend_aarch64_global_int_pointer_roundtrip_plan.md

## Purpose

Turn the next open backend child idea into an executable runbook for one bounded AArch64 pointer round-trip slice.

## Goal

Complete the bounded AArch64 global-int pointer-roundtrip seam identified in the source idea.

## Core Rule

Keep this plan narrow. Do not broaden into generic pointer round-tripping, general address arithmetic, or unrelated backend initiatives.

## Read First

- [ideas/open/14_backend_aarch64_global_int_pointer_roundtrip_plan.md](/Users/chi-shengwu/c4c/ideas/open/14_backend_aarch64_global_int_pointer_roundtrip_plan.md)
- [ideas/open/__backend_port_plan.md](/Users/chi-shengwu/c4c/ideas/open/__backend_port_plan.md)
- `ref/claudes-c-compiler/src/backend/`
- `src/backend/`
- `src/codegen/lir/`

## Current Target

- Backend-owned follow-on: bounded AArch64 global int-pointer-roundtrip support
- Reference strategy: mechanical C++ port where possible
- Execution target: Linux-produced binaries

## Non-Goals

- Do not redesign existing LIR data structures just to simplify this slice.
- Do not mix built-in assembler or linker work into this plan.
- Do not absorb new backend initiatives silently; record them under `ideas/open/` instead.
- Do not treat this slice as generic `ptrtoint` or `inttoptr` enablement.

## Working Model

- Treat the source child idea as the contract and the umbrella idea as roadmap context.
- Prefer adapter or shim work over early shared-IR redesign.
- Keep target-specific behavior isolated to AArch64 surfaces.

## Execution Rules

- Compare behavior against the reference backend before inventing new structure.
- Keep patches mechanical and reviewable.
- Preserve existing backend boundaries unless the reference seam requires a small attachment-layer divergence.
- If this work uncovers a separate initiative, write a new `ideas/open/*.md` file and switch plans instead of expanding this runbook.

## Steps

### Step 1: Reconstruct the Missing Narrow Slice

Goal: recover the exact implementation seam for the global-int round-trip follow-on from the repo and runtime context.

Actions:

- inspect existing backend AArch64 follow-on artifacts, tests, and fallback output for `global_int_pointer_roundtrip`
- identify the precise bounded matcher shape shared by the synthetic fixture and runtime-emitted LIR
- record the concrete scope in `plan_todo.md`

Completion check:

- the exact missing seam is described concretely enough to drive a single test-first implementation slice

### Step 2: Add or Update Targeted Coverage

Goal: capture the intended bounded round-trip behavior before code changes.

Actions:

- add or update the narrowest backend-facing test that exercises global-int pointer round-tripping on AArch64
- confirm the test fails for the expected reason before implementation

Completion check:

- a targeted failing test exists and isolates this slice

### Step 3: Implement the Backend Slice

Goal: make the active AArch64 backend path lower or preserve bounded global-int pointer round-tripping correctly.

Actions:

- port or adapt the minimal reference mechanism into `src/backend/`
- keep architecture-specific code isolated
- avoid unrelated cleanup

Completion check:

- the targeted test passes with a minimal backend-scoped change

### Step 4: Validate and Preserve Lifecycle State

Goal: prove the slice did not regress broader behavior and leave the next agent clear state.

Actions:

- run targeted verification, nearby backend tests, and broader regression checks required by the execution prompt
- update `plan_todo.md` with completed work, next slice, and blockers
- if this slice completes, close the child idea rather than mutating the umbrella roadmap ad hoc

Completion check:

- validation status and next-step state are both reflected in `plan_todo.md`
