# Built-in AArch64 Assembler Runbook

Status: Active
Source Idea: ideas/open/06_backend_builtin_assembler_aarch64_plan.md
Activated from: ideas/open/06_backend_builtin_assembler_aarch64_plan.md

## Purpose

Turn the compile-integrated AArch64 assembler boundary into the first object-emitting slice for the currently supported backend subset, without expanding into linker work or broad instruction coverage.

## Goal

Route one bounded AArch64 backend-emitted assembly slice through the in-tree parser, encoder, and ELF-writer path and start producing comparable ELF relocatable objects.

## Core Rule

Do not broaden this runbook into general assembler completion. Keep the first slice tied to the backend subset already emitted today and prove it with narrow object-level comparisons.

## Read First

- [ideas/open/06_backend_builtin_assembler_aarch64_plan.md](/Users/chi-shengwu/c4c/ideas/open/06_backend_builtin_assembler_aarch64_plan.md)
- [ideas/closed/05_backend_builtin_assembler_boundary_plan.md](/Users/chi-shengwu/c4c/ideas/closed/05_backend_builtin_assembler_boundary_plan.md)
- [ideas/open/__backend_port_plan.md](/Users/chi-shengwu/c4c/ideas/open/__backend_port_plan.md)
- [BINARY_UTILS_CONTRACT.md](/Users/chi-shengwu/c4c/src/backend/aarch64/BINARY_UTILS_CONTRACT.md)
- `ref/claudes-c-compiler/src/backend/arm/assembler/README.md`
- `ref/claudes-c-compiler/src/backend/arm/assembler/mod.rs`
- `ref/claudes-c-compiler/src/backend/arm/assembler/parser.rs`
- `ref/claudes-c-compiler/src/backend/arm/assembler/elf_writer.rs`
- `ref/claudes-c-compiler/src/backend/arm/assembler/encoder/mod.rs`
- `ref/claudes-c-compiler/src/backend/arm/assembler/encoder/data_processing.rs`
- `ref/claudes-c-compiler/src/backend/arm/assembler/encoder/compare_branch.rs`
- `ref/claudes-c-compiler/src/backend/arm/assembler/encoder/load_store.rs`
- `ref/claudes-c-compiler/src/backend/arm/assembler/encoder/system.rs`
- `ref/claudes-c-compiler/src/backend/arm/assembler/encoder/bitfield.rs`

## Current Targets

- keep the ref three-stage shape visible: parse text, encode instructions plus relocations, write ELF64 relocatable objects
- restrict the first object-emitting slice to backend-emitted AArch64 assembly that already exists in repo tests
- make one bounded backend path flow through the in-tree assembler instead of only preserving staged text
- compare emitted object metadata against the external assembler baseline for representative cases

## Non-Goals

- linker implementation
- x86-64 or rv64 assembler work
- broad parser or encoder coverage beyond the currently emitted AArch64 backend subset
- dynamic-linking, TLS, GOT, IFUNC, large NEON, or unrelated relocation expansion

## Working Model

- preserve the text-first assembler boundary closed in the previous runbook
- reuse shared ELF support where the repo already mirrors the ref layout
- bias toward the smallest supported backend subset first:
  return, direct call, compare and branch, stack-relative loads and stores, `adrp` plus low12 materialization, and narrow data directives
- keep the external assembler tests as the comparison oracle while built-in object emission is incomplete

## Execution Rules

- add or tighten narrow tests before expanding parser, encoder, or ELF-writer behavior
- prefer one backend-emitted object slice at a time instead of partial genericity
- keep parser, encoder, and writer seams explicit so later relocation and linker plans can target them directly
- if a required capability is adjacent but not necessary for the first object-emitting slice, record it back into the idea instead of silently widening this runbook

## Step 1: Inventory The Minimal Object-Emission Slice

Goal: identify the smallest backend-emitted AArch64 assembly case that can move from staged text to emitted object bytes.

Primary targets:

- `src/backend/aarch64/assembler/parser.*`
- `src/backend/aarch64/assembler/encoder/`
- `src/backend/aarch64/assembler/elf_writer.cpp`
- backend contract and object-coverage tests

Actions:

- inspect which current backend-emitted cases are already locked by object-contract tests
- map the exact directives, instruction forms, symbol records, and relocations needed by the narrowest case
- confirm which parser and writer stubs already exist versus which minimal pieces are still placeholders

Completion check:

- one concrete first object-emission slice is named, with its required directives, instructions, and relocation expectations enumerated

## Step 2: Connect The Boundary To Real Assembler State

Goal: make the chosen slice travel through parser, encoder, and ELF-writer state instead of only echoing staged text.

Primary targets:

- `src/backend/aarch64/assembler/mod.*`
- `src/backend/aarch64/assembler/parser.*`
- `src/backend/aarch64/assembler/encoder/`
- `src/backend/aarch64/assembler/elf_writer.cpp`

Actions:

- replace the current stub-only path with the smallest real parse and encode flow needed by the chosen slice
- keep the result contract explicit about whether object bytes were emitted and what output path was targeted
- preserve the compatibility overload only as a shim over the named request/result seam

Completion check:

- the chosen backend-emitted assembly slice reaches real parser, encoder, and writer state through the active assembler entry point

## Step 3: Emit And Compare One Real Object Slice

Goal: start producing one comparable ELF relocatable object for the supported subset.

Primary targets:

- assembler smoke and object-comparison tests
- backend-to-assembler handoff helper
- external-object comparison fixtures under `tests/c/internal/`

Actions:

- add or tighten one object-level test that routes a real backend-emitted slice through the built-in assembler path
- validate section inventory, symbol records, relocation inventory, and bounded disassembly shape against the external assembler baseline
- keep the first slice small; split follow-on parser or relocation work into later iterations if the comparison surface grows

Completion check:

- one backend-emitted AArch64 slice produces a comparable ELF relocatable object through the built-in assembler path

## Acceptance Checks

- parser, encoder, and ELF-writer pieces are compile-integrated behind one active assembler entry point
- one bounded backend-emitted AArch64 slice flows through the in-tree assembler without ad hoc local adapters
- the first emitted object is checked against external-assembler output on sections, symbols, relocations, or bounded disassembly
