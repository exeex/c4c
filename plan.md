# Backend Linker Object IO Runbook

Status: Active
Source Idea: ideas/open/07_backend_linker_object_io_plan.md
Activated from: ideas/open/07_backend_linker_object_io_plan.md

## Purpose

Turn the mirrored shared ELF and linker-common trees into the first reusable object/archive input layer for later linker work, without expanding into relocation application or final executable emission.

## Goal

Parse one bounded relocatable ELF object and one bounded archive case through shared linker infrastructure so later AArch64 linker work can consume stable object, section, symbol, and relocation data.

## Core Rule

Do not broaden this runbook into executable linking. Keep the active slice centered on shared object and archive IO surfaces only.

## Read First

- [ideas/open/07_backend_linker_object_io_plan.md](/Users/chi-shengwu/c4c/ideas/open/07_backend_linker_object_io_plan.md)
- [ideas/closed/06_backend_builtin_assembler_aarch64_plan.md](/Users/chi-shengwu/c4c/ideas/closed/06_backend_builtin_assembler_aarch64_plan.md)
- [ideas/open/08_backend_builtin_linker_aarch64_plan.md](/Users/chi-shengwu/c4c/ideas/open/08_backend_builtin_linker_aarch64_plan.md)
- [ideas/open/__backend_port_plan.md](/Users/chi-shengwu/c4c/ideas/open/__backend_port_plan.md)
- `ref/claudes-c-compiler/src/backend/linker_common/README.md`
- `ref/claudes-c-compiler/src/backend/linker_common/types.rs`
- `ref/claudes-c-compiler/src/backend/linker_common/parse_object.rs`
- `ref/claudes-c-compiler/src/backend/linker_common/archive.rs`
- `ref/claudes-c-compiler/src/backend/linker_common/symbols.rs`
- `ref/claudes-c-compiler/src/backend/linker_common/merge.rs`
- `ref/claudes-c-compiler/src/backend/linker_common/section_map.rs`
- `ref/claudes-c-compiler/src/backend/elf/archive.rs`
- `ref/claudes-c-compiler/src/backend/elf/io.rs`
- `ref/claudes-c-compiler/src/backend/elf/constants.rs`

## Current Targets

- keep ELF object and archive parsing target-independent under `src/backend/elf/` and `src/backend/linker_common/`
- expose stable shared representations for sections, symbols, relocations, and archive members
- prove one bounded happy-path object case and one bounded archive case before widening malformed-input coverage

## Non-Goals

- relocation application
- final executable writing
- target-specific linker policy
- broad linker orchestration beyond what object and archive parsing requires

## Working Model

- mirror the ref split between low-level ELF reading and shared linker-facing object bookkeeping
- keep parser outputs explicit enough that later `aarch64/linker/` code can consume them without reparsing raw bytes
- prefer one bounded parsing surface at a time: first relocatable object inventory, then archive member discovery

## Execution Rules

- add or tighten the narrowest tests before expanding parser coverage
- compare parsed inventories against known object and archive fixtures instead of guessing ELF details
- record follow-on relocation or layout work back into open ideas instead of widening this runbook

## Step 1: Lock The Minimal Shared Object IO Slice

Goal: identify the smallest relocatable object case that exercises shared section, symbol, and relocation parsing.

Primary targets:

- `src/backend/elf/`
- `src/backend/linker_common/parse_object.*`
- shared object-coverage tests under `tests/`

Actions:

- inspect current compile-integrated shared ELF and linker-common surfaces
- choose one bounded relocatable object fixture with `.text`, symbol table, and at least one relocation inventory expectation
- enumerate the exact shared types and parser seams that the first object slice must populate

Completion check:

- one concrete first object-parsing slice is named, with required section, symbol, and relocation expectations recorded

## Step 2: Parse One Real Relocatable Object Through Shared Types

Goal: make one bounded ELF relocatable object parse into stable shared linker data structures.

Primary targets:

- `src/backend/elf/`
- `src/backend/linker_common/types.*`
- `src/backend/linker_common/parse_object.*`

Actions:

- replace placeholder or partial object-reading paths only as needed for the chosen slice
- keep shared object representations explicit about sections, symbols, and relocations
- avoid introducing target-local parsing logic into the shared layer

Completion check:

- one real relocatable object parses through shared ELF/linker-common code with test-checked section, symbol, and relocation inventories

## Step 3: Add The First Archive IO Slice

Goal: make one bounded archive case parse far enough for later symbol-driven member discovery.

Primary targets:

- `src/backend/elf/archive.*`
- `src/backend/linker_common/archive.*`
- archive-focused tests under `tests/`

Actions:

- add or tighten one narrow archive test before implementing archive-member parsing
- parse one single-member or small archive case into shared archive/member structures
- validate member enumeration and symbol-driven lookup without widening into full linking

Completion check:

- one bounded archive case parses through the shared layer with test-checked member inventory and lookup behavior

## Acceptance Checks

- `src/backend/elf/` and `src/backend/linker_common/` stay compile-integrated as shared infrastructure
- one bounded relocatable object parses into stable shared object data
- one bounded archive case parses into stable shared archive/member data
- later AArch64 linker work can consume these shared surfaces without reparsing raw inputs
