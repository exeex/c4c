# Native Object Emission Umbrella Research Runbook

Status: Active
Source Idea: ideas/open/329_native_object_emission_umbrella.md

## Purpose

Turn the native object emission umbrella into a researched child-idea queue
without implementing backend object emission in this runbook.

## Goal

Map the current RV64 and AArch64 emission architecture, compare the relevant
reference assembler/object-writer material, create focused child ideas under
`ideas/open/`, then switch lifecycle state to the first child idea.

## Core Rule

This is a research-only decomposition plan. Do not edit backend implementation,
tests, CLI behavior, runtime wiring, or c-testsuite expectations while this
umbrella is active.

## Read First

- `ideas/open/329_native_object_emission_umbrella.md`
- `src/backend/mir/riscv/`
- `src/backend/mir/aarch64/`
- `src/backend/mir/riscv/assembler/`
- `ref/claudes-c-compiler/src/backend/riscv/assembler/`
- backend runtime and c-testsuite wiring discovered during inspection

## Current Scope

- Inspect current RV64 and AArch64 backend emission flows.
- Inspect backend runtime and c-testsuite backend route wiring.
- Inspect the partial C++ RV64 assembler/object writer port.
- Compare only relevant reference design pieces.
- Create child ideas with concrete ownership boundaries.
- Prepare lifecycle transition to the first child idea after the queue exists.

## Non-Goals

- Do not implement `.o` emission in this umbrella plan.
- Do not replace or weaken the existing `.s` route.
- Do not make c-testsuite default to object output.
- Do not build a GNU-compatible textual assembler.
- Do not absorb unrelated backend semantic fixes into this plan.
- Do not copy the reference assembler wholesale.

## Working Model

The desired long-term shape is:

```text
backend machine model -> object writer -> .o
backend machine model -> asm writer -> .s
c4c-as .s -> object writer -> .o
```

The compiler's primary direct `.o` path must not depend on printing `.s` and
parsing it back. The research should identify the shared data model needed for
encoded instructions, sections, symbols, labels, and typed relocations.

## Execution Rules

- Keep findings in `todo.md` until they must become child source ideas.
- Preserve the umbrella source idea unless its durable intent changes.
- Child ideas must include narrow goals, owned files or subsystems where known,
  explicit non-goals, testing/proof expectations, reviewer reject signals, and
  dependency notes.
- Child ideas must be independently closable; do not create one child that
  implements the whole object pipeline.
- Treat missing RV64 or AArch64 ownership as a blocker.

## Step 1: Inspect Current Backend Emission Architecture

Goal: understand how RV64 and AArch64 currently lower backend machine state to
assembly text and where object emission could integrate.

Primary targets:

- `src/backend/mir/riscv/`
- `src/backend/mir/aarch64/`
- current backend codegen route and prepared-module emission paths

Actions:

- Identify the current asm writer entry points and data structures.
- Identify whether RV64 and AArch64 already share emission abstractions.
- Record seams that can publish encoded instructions, labels, symbols, and
  relocations without text round-tripping.
- Record route differences that child ideas must preserve explicitly.

Completion check:

- `todo.md` summarizes the current RV64/AArch64 emission map and candidate
  object-emission seams without editing implementation files.

## Step 2: Inspect Runtime And Backend Test Wiring

Goal: understand how backend outputs are compiled, linked, and tested today.

Primary targets:

- backend runtime support discovered in the repo
- c-testsuite backend route scripts and CTest labels
- existing asm-route backend tests

Actions:

- Identify where `.s` output enters clang/as or runtime linking.
- Identify how backend tests distinguish route, target, and output form.
- Record where future object-route tests can sit beside asm-route tests.
- Preserve existing asm-route coverage requirements.

Completion check:

- `todo.md` identifies the test and runtime integration points needed for child
  ideas, including proof commands the supervisor can delegate later.

## Step 3: Inspect Existing RV64 Assembler/Object Port

Goal: understand the partial C++ RV64 assembler/object writer port already in
the repo and decide whether it is reusable, incomplete, or only reference
material.

Primary target:

- `src/backend/mir/riscv/assembler/`

Actions:

- Identify public API shape, encoded instruction records, relocation records,
  section/symbol handling, and known gaps.
- Compare these surfaces to current RV64 backend emission output.
- Record what can be reused directly and what requires redesign.

Completion check:

- `todo.md` lists the port's reusable pieces, missing pieces, and integration
  risks without changing the port.

## Step 4: Compare Relevant Reference Design Pieces

Goal: extract only design guidance from the reference implementation that
helps c4c's native object route.

Primary target:

- `ref/claudes-c-compiler/src/backend/riscv/assembler/`

Actions:

- Review typed `EncodeResult` records, target relocation enums, RISC-V
  `pcrel_hi` / `pcrel_lo` pairing, ELF section/symbol/relocation writing, and
  RVC/linker-relaxation decisions.
- Map reference concepts to c4c-owned APIs instead of copying code wholesale.
- Note which ideas are RV64-specific and which generalize to AArch64 or shared
  object emission.

Completion check:

- `todo.md` records the relevant reference concepts and rejects text-assembler
  dependency for the compiler's primary object path.

## Step 5: Create Focused Child Ideas

Goal: convert the research into a child queue under `ideas/open/`.

Actions:

- Create child ideas for at least:
  - native object model and emission API design
  - RV64 minimal relocatable ELF object emission
  - AArch64 minimal relocatable ELF object emission
  - CLI and test integration for `--codegen obj`
  - broader c-testsuite object-route scan and default-switch readiness
  - textual assembler follow-up, if still needed after direct `.o` design
- Include concrete ownership, non-goals, proof expectations, reject signals,
  and dependency notes in each child idea.
- Keep each child idea small enough to close independently.

Completion check:

- The child queue exists under `ideas/open/`, covers both RV64 and AArch64, and
  preserves asm-route compatibility expectations.

## Step 6: Switch Lifecycle State To The First Child Idea

Goal: stop using the umbrella as the active execution plan once the child queue
exists.

Actions:

- Ask the plan owner to deactivate or replace this umbrella runbook.
- Activate the first child idea as the new `plan.md` / `todo.md` pair.
- Leave the umbrella open until all child ideas close and final acceptance
  review confirms they compose into native object emission readiness.

Completion check:

- `plan.md` and `todo.md` point to the first child idea, not this umbrella, and
  `ideas/open/329_native_object_emission_umbrella.md` remains open.
