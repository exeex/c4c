# Built-in Assembler Boundary Runbook

Status: Active
Source Idea: ideas/open/05_backend_builtin_assembler_boundary_plan.md

## Purpose

Turn the mirrored assembler tree from a loose collection of translated files into an explicit, compile-integrated boundary that later AArch64 object emission work can target without ad hoc adapters.

## Goal

Choose and stage one narrow assembler entry contract, biased toward the existing text-first `parse -> encode -> write ELF` shape used by the reference backend.

## Core Rule

Do not implement full AArch64 object emission in this runbook. This plan is about compile integration and boundary definition, not full instruction coverage.

## Read First

- [ideas/open/05_backend_builtin_assembler_boundary_plan.md](/Users/chi-shengwu/c4c/ideas/open/05_backend_builtin_assembler_boundary_plan.md)
- [ideas/open/__backend_port_plan.md](/Users/chi-shengwu/c4c/ideas/open/__backend_port_plan.md)
- [BINARY_UTILS_CONTRACT.md](/Users/chi-shengwu/c4c/src/backend/aarch64/BINARY_UTILS_CONTRACT.md)
- `ref/claudes-c-compiler/src/backend/README.md`
- `ref/claudes-c-compiler/src/backend/asm_preprocess.rs`
- `ref/claudes-c-compiler/src/backend/asm_expr.rs`
- `ref/claudes-c-compiler/src/backend/elf/`
- `ref/claudes-c-compiler/src/backend/elf_writer_common.rs`
- `ref/claudes-c-compiler/src/backend/arm/assembler/mod.rs`
- `ref/claudes-c-compiler/src/backend/arm/assembler/parser.rs`
- `ref/claudes-c-compiler/src/backend/arm/assembler/elf_writer.rs`

## Current Targets

- make the mirrored shared and AArch64 assembler files compile behind one bounded include surface
- decide whether the assembler boundary stays raw-text-first or introduces a structured internal form
- declare the data flow from current backend assembly text into the future assembler entry point
- lock the chosen boundary with narrow tests instead of implicit conventions

## Non-Goals

- full instruction encoding
- linker work
- broad backend redesign
- introducing a target-generic instruction IR unless the evidence clearly requires it

## Working Model

- preserve external-toolchain comparability as the default
- prefer a text-first boundary unless compile-integration evidence forces a different choice
- mirror ref shared helpers where the ref already centralizes behavior
- keep the staged contract narrow enough that later AArch64 assembler work can extend it mechanically

## Execution Rules

- add or tighten narrow tests before expanding the contract surface
- separate shared assembler helper seams from target-local parser or encoder seams
- document the chosen boundary in repo-local headers or notes, not only in test expectations
- if implementation pressure suggests a second internal IR, record the evidence explicitly before accepting that scope

## Step 1: Inventory The Current Assembler Entry Shape

Goal: map the actual assembly-text subset and the current staged assembler surfaces that future work can target.

Primary targets:

- `src/backend/aarch64/codegen/`
- `src/backend/aarch64/assembler/`
- `src/backend/asm_preprocess.cpp`
- `src/backend/asm_expr.cpp`
- `src/backend/elf/`
- `src/backend/elf_writer_common.cpp`

Actions:

- inspect which backend-emitted AArch64 assembly constructs are already locked by tests and contract notes
- identify which shared helper seams from ref are already mirrored, stubbed, or missing
- record the current parse and object-writer entry points the future assembler boundary could expose

Completion check:

- the repo has a concrete map of the current text-emission inputs and the staged assembler helper surfaces that can be built on

## Step 2: Decide And Declare The Boundary

Goal: choose one explicit assembler entry boundary and stage it in code.

Primary targets:

- shared assembler-facing headers
- AArch64 assembler module entry
- any minimal adapter from backend-emitted text into that entry

Actions:

- choose whether the active boundary remains raw GNU-style assembly text or a narrowly structured form
- keep the default bias toward the ref text-first pipeline unless concrete duplication argues otherwise
- add or tighten the minimal declarations that make the chosen boundary include-reachable
- record the decision in the relevant contract note if code alone would be too implicit

Completion check:

- one assembler entry boundary is explicit in code and documentation, and later target work can name it directly

## Step 3: Prove Compile Integration

Goal: make the chosen boundary testable and build-clean without implementing full encoding behavior.

Primary targets:

- assembler smoke tests
- include/build wiring
- one bounded backend-to-assembler handoff slice

Actions:

- add or tighten a narrow smoke test that exercises the chosen boundary
- ensure shared helpers and AArch64 assembler files compile together through the staged include surface
- validate that one existing backend-emitted function can be threaded to the chosen boundary without one-off local seams

Completion check:

- the assembler tree compiles behind the chosen boundary and one bounded handoff slice is validated

## Acceptance Checks

- the mirrored assembler tree compiles and shares one coherent declaration surface
- the chosen assembler input boundary is explicit in code and repo-local documentation
- later AArch64 assembler work can target a stable interface instead of rediscovering the boundary
