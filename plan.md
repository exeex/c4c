# RV64 Object Emission Cleanup Runbook

Status: Active
Source Idea: ideas/open/519_rv64_object_emission_cleanup_umbrella.md

## Purpose

Analyze `src/backend/mir/riscv/codegen/object_emission.cpp` and produce a
staged, behavior-preserving cleanup plan for splitting RV64 object emission
ownership into reviewable follow-up ideas.

## Goal

Create a durable cleanup artifact and concrete `ideas/open/` follow-up source
ideas without moving implementation code in this umbrella.

## Core Rule

This is an analysis umbrella. Do not edit RV64 implementation files, tests,
expectations, allowlists, unsupported markers, runtime comparison, or pass/fail
accounting while executing this plan.

## Read First

- `ideas/open/519_rv64_object_emission_cleanup_umbrella.md`
- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `src/backend/mir/aarch64/codegen/`
- `.codex/skills/c4c-clang-tools/SKILL.md`

## Current Targets

- Primary source file:
  `src/backend/mir/riscv/codegen/object_emission.cpp`
- Existing RV64 destination candidates:
  - `src/backend/mir/riscv/codegen/prepared_call_emit.cpp`
  - `src/backend/mir/riscv/codegen/prepared_frame_emit.cpp`
  - `src/backend/mir/riscv/codegen/prepared_function_emit.cpp`
  - `src/backend/mir/riscv/codegen/prepared_global_memory_emit.cpp`
  - `src/backend/mir/riscv/codegen/prepared_local_memory_emit.cpp`
  - `src/backend/mir/riscv/codegen/prepared_scalar_emit.cpp`
  - `src/backend/mir/riscv/codegen/prepared_edge_publication_emit.cpp`
  - `src/backend/mir/riscv/codegen/rv64_line_assembler.cpp`
  - `src/backend/mir/riscv/codegen/returns.cpp`
  - `src/backend/mir/riscv/codegen/prologue.cpp`
  - `src/backend/mir/riscv/codegen/variadic.cpp`
  - `src/backend/mir/riscv/codegen/calls.cpp`
  - `src/backend/mir/riscv/codegen/memory.cpp`
  - `src/backend/mir/riscv/codegen/globals.cpp`
- Durable analysis artifact:
  `docs/rv64_object_emission_cleanup/` or
  `build/agent_state/519_rv64_object_emission_cleanup/`

## Non-Goals

- Do not move or rewrite RV64 implementation code in this umbrella.
- Do not change instruction encodings, ELF/object emission, relocations,
  stack-frame layout, prepared handoff admission, diagnostics, runtime
  behavior, or gcc_torture evidence.
- Do not repair gcc_torture failures.
- Do not add target-side inference for missing prepared or BIR facts.
- Do not copy AArch64 structure mechanically when RV64 ownership differs.

## Working Model

- Use AST-backed structure queries before raw long-file reading.
- Treat AArch64 codegen layout as a comparison point, not a template to copy.
- Prefer existing RV64 focused files when they already express ownership.
- Create new destination files only when existing files do not match the
  ownership boundary.
- Materialize follow-up work as concrete `ideas/open/*.md` files before close.

## Execution Rules

- Keep all routine progress and proof notes in `todo.md`.
- Keep analysis output in the chosen durable artifact directory.
- Preserve source-idea intent; do not edit the source idea unless lifecycle
  closure or a true source-intent correction requires it.
- Every proposed follow-up idea must name owned files, behavior-preserving
  scope, validation command expectations, and reviewer reject signals.
- Reject any route that hides monolithic coupling behind new filenames without
  clear caller boundaries.

## Step 1: Establish Structure Baseline

Goal: inventory the current RV64 object emission surface without making code
changes.

Primary target:
`src/backend/mir/riscv/codegen/object_emission.cpp`

Actions:

- Load and follow `.codex/skills/c4c-clang-tools/SKILL.md`.
- Confirm `c4c-clang-tool` and `c4c-clang-tool-ccdb` availability.
- Record current line count for `object_emission.cpp`.
- Use AST-backed symbol, function-signature, caller/callee, and type-reference
  queries to identify major helper families and dependency clusters.
- Create the durable artifact directory if needed.
- Record the commands used and the clusters observed.

Completion check:

- The artifact contains line-count, tool availability, query commands, and an
  initial region/dependency map.
- No implementation files are changed.

## Step 2: Compare Against AArch64 Codegen Layout

Goal: identify which AArch64 ownership boundaries are useful references for
RV64 and where RV64 prepared-object flow should differ.

Primary targets:

- `src/backend/mir/aarch64/codegen/`
- RV64 codegen files listed in Current Targets

Actions:

- Inventory AArch64 codegen organization around calls, memory, globals,
  returns, prologue, variadic, object emission, and module compile.
- Compare those boundaries with the RV64 region map from Step 1.
- Record where RV64 should reuse existing destination files.
- Record where RV64 should intentionally diverge from AArch64.

Completion check:

- The artifact contains an AArch64 comparison table and an RV64 destination
  map draft.
- Risk notes cover symbol/fixup handling, prepared admission, and runtime
  behavior.

## Step 3: Draft Staged Cleanup Follow-Ups

Goal: convert the region map into behavior-preserving follow-up idea slices.

Actions:

- Order candidate slices so low-risk pure helpers move first and
  cross-cutting symbol/fixup or final module assembly ownership moves late or
  stays central.
- Prefer slices such as:
  - pure encoding and byte/fixup append helpers;
  - prepared admission and diagnostics;
  - local/global memory;
  - scalar operations;
  - call, variadic, prologue, and return handling;
  - object data;
  - final object-module assembly boundaries.
- Define owned files and validation commands for each future slice.
- Mark dependencies between slices when one split must precede another.

Completion check:

- The artifact contains a staged follow-up list with owned files,
  prerequisites, risks, and validation expectations for each slice.
- The list remains analysis-only and behavior-preserving.

## Step 4: Materialize Follow-Up Ideas

Goal: create concrete open source ideas for the staged cleanup work.

Actions:

- Add one or more `ideas/open/*.md` files for the staged follow-up cleanup
  slices.
- Each source idea must include goal, why it exists, in-scope work,
  out-of-scope work, acceptance criteria, and concrete reviewer reject signals.
- Link each created idea from the durable artifact.
- Keep the follow-up ideas narrow enough for execution agents to validate and
  review independently.

Completion check:

- The staged list has matching concrete `ideas/open/*.md` follow-up files.
- No follow-up idea mixes behavior-preserving movement with RV64 capability
  repair or gcc_torture expectation changes.

## Step 5: Close Readiness Review

Goal: decide whether idea 519 is complete and ready for lifecycle close.

Actions:

- Verify the artifact includes:
  - current line-count and region map;
  - `c4c-clang-tools` query notes;
  - AArch64 comparison table;
  - proposed RV64 destination map;
  - proposed new-file list, if any;
  - staged follow-up idea list with owned files and validation commands;
  - links to concrete `ideas/open/` follow-up ideas;
  - risk notes for symbol/fixup handling, prepared admission, and runtime
    behavior.
- Confirm no implementation files changed.
- Run default CTest only if the lifecycle close packet or supervisor requires
  close-time regression proof.

Completion check:

- The source idea acceptance criteria are satisfied.
- The active plan can be closed by the plan owner after the required lifecycle
  close gate passes.
