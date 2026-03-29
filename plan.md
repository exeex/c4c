# Built-in AArch64 Linker Runbook

Status: Active
Source Idea: ideas/open/08_backend_builtin_linker_aarch64_plan.md
Activated from: ideas/open/08_backend_builtin_linker_aarch64_plan.md

## Purpose

Turn the mirrored shared linker layers plus `src/backend/aarch64/linker/` into the first bounded static-link path for AArch64, without widening into dynamic-linking or full Linux system-linking work.

## Goal

Link one tiny multi-object AArch64 program through the built-in linker so later linker work can build on explicit object loading, symbol resolution, relocation application, and ELF executable emission seams.

## Core Rule

Keep this runbook static-link-first and narrow. Do not widen into shared-library, PLT/GOT, TLS, or full stdlib-linking work.

## Read First

- [ideas/open/08_backend_builtin_linker_aarch64_plan.md](/Users/chi-shengwu/c4c/ideas/open/08_backend_builtin_linker_aarch64_plan.md)
- [ideas/closed/07_backend_linker_object_io_plan.md](/Users/chi-shengwu/c4c/ideas/closed/07_backend_linker_object_io_plan.md)
- [ideas/closed/06_backend_builtin_assembler_aarch64_plan.md](/Users/chi-shengwu/c4c/ideas/closed/06_backend_builtin_assembler_aarch64_plan.md)
- [ideas/open/__backend_port_plan.md](/Users/chi-shengwu/c4c/ideas/open/__backend_port_plan.md)
- `ref/claudes-c-compiler/src/backend/arm/linker/README.md`
- `ref/claudes-c-compiler/src/backend/arm/linker/link.rs`
- `ref/claudes-c-compiler/src/backend/arm/linker/input.rs`
- `ref/claudes-c-compiler/src/backend/arm/linker/types.rs`
- `ref/claudes-c-compiler/src/backend/arm/linker/reloc.rs`
- `ref/claudes-c-compiler/src/backend/arm/linker/emit_static.rs`
- `ref/claudes-c-compiler/src/backend/linker_common/`

## Current Targets

- keep orchestration under `src/backend/aarch64/linker/` separated from shared IO in `src/backend/elf/` and `src/backend/linker_common/`
- reuse the shared object/archive parser for bounded input loading instead of reparsing AArch64 objects locally
- prove one tiny static executable path with a relocation-bearing multi-object case
- make the first executable emission surfaces explicit enough that later tests can compare them with the external linker path

## Non-Goals

- dynamic linking
- shared-library output
- PLT/GOT, TLS, IFUNC, or `.eh_frame_hdr` follow-ons
- broad archive-resolution generalization beyond the first bounded static-link slice
- full Linux stdlib linking

## Working Model

- mirror the ref split between:
  input loading,
  shared symbol registration and object/archive bookkeeping,
  AArch64 relocation application,
  and static ELF emission
- prefer one bounded executable path first: tiny multi-object input, one known relocation-bearing reference, and one narrow executable layout proof
- keep target-specific work in `src/backend/aarch64/linker/`; only move code into shared layers when the logic is genuinely target-independent

## Execution Rules

- add or tighten the narrowest linker test before expanding implementation
- validate linked output by inspecting executable layout and, when practical, comparing against the external linker path for the same tiny program
- record follow-on linker features back into open ideas instead of widening this runbook

## Step 1: Lock The First Static-Link Slice

Goal: choose the smallest relocation-bearing multi-object AArch64 case that exercises the first built-in linker path end to end.

Primary targets:

- `src/backend/aarch64/linker/`
- linker-focused tests under `tests/`

Actions:

- inspect the current `aarch64/linker/` stubs, entry points, and tests
- choose one tiny two-object or object-plus-archive case with an explicit call or global-reference relocation
- record the exact expected outputs for input loading, symbol resolution, relocation application, and executable sections

Completion check:

- one concrete first static-link slice is named, with explicit input and output expectations recorded

## Step 2: Connect Shared Input Loading To AArch64 Linker Entry Points

Goal: make the AArch64 linker subtree consume the shared object/archive data surfaces through explicit entry points.

Primary targets:

- `src/backend/aarch64/linker/input.*`
- `src/backend/aarch64/linker/elf.*`
- `src/backend/aarch64/linker/types.*`
- `src/backend/linker_common/`

Actions:

- replace placeholder or commented shared-linker wiring only as needed for the chosen slice
- keep shared object/archive parsing in the shared layer and expose only the data the AArch64 linker needs
- avoid mixing relocation semantics into the input-loading step

Completion check:

- the chosen bounded input case loads through shared object/archive surfaces into the AArch64 linker entry path

## Step 3: Apply The First AArch64 Static-Link Path

Goal: resolve symbols, apply the narrow relocation subset, and emit one bounded static executable.

Primary targets:

- `src/backend/aarch64/linker/link.*`
- `src/backend/aarch64/linker/reloc.*`
- `src/backend/aarch64/linker/emit_static.*`
- linker-focused tests under `tests/`

Actions:

- implement only the relocation kinds required by the chosen case
- emit the minimal executable layout needed for the first tiny linked program
- compare the resulting layout or behavior against the external linker path when the test shape supports it

Completion check:

- one bounded multi-object AArch64 case links into a test-checked static executable through the built-in linker path

## Acceptance Checks

- `src/backend/aarch64/linker/` compiles against the shared ELF/linker layers
- the first static-link slice uses shared object/archive inputs instead of local reparsing
- one bounded AArch64 executable path resolves symbols, applies the required relocations, and emits a stable executable surface
- dynamic-linking and broader linker policy remain out of scope
