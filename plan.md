# Active Plan: x86 External Call Object Relocation

Status: Active
Source Idea: ideas/open/33_backend_x86_external_call_object_plan.md
Supersedes: ideas/open/32_backend_builtin_assembler_x86_call_relocation_plan.md

## Purpose

Repair the x86 relocation-bearing assembler slice by switching from the
relocation-free local `call_helper.c` fixture to one bounded backend-owned
external-call object contract that actually exercises the existing linker path.

## Goal

Make the built-in x86 assembler emit one relocation-bearing object for an
undefined helper call, then prove that object matches the staged
`R_X86_64_PLT32` linker contract.

## Core Rule

Keep this slice strictly limited to one undefined-helper call shape and one
`.text` relocation entry. Do not widen into generic relocation coverage, new
linker behavior, or unrelated runtime-fixture retargeting.

## Read First

- `ideas/open/33_backend_x86_external_call_object_plan.md`
- `ideas/open/32_backend_builtin_assembler_x86_call_relocation_plan.md`
- `ideas/open/__backend_port_plan.md`
- `ideas/closed/23_backend_builtin_assembler_x86_plan.md`
- `ideas/closed/24_backend_builtin_linker_x86_plan.md`
- `prompts/AGENT_PROMPT_EXECUTE_PLAN.md`

## Current Targets / Scope

- `src/backend/x86/assembler/`
- `src/backend/x86/codegen/`
- `src/backend/x86/codegen/emit.hpp`
- x86 backend adapter / assembler object tests covering one undefined helper
  call
- one bounded x86 backend fixture or asm path that emits `call helper_ext` (or
  equivalent undefined helper symbol)

## Non-Goals

- generic x86 relocation completeness
- new linker features beyond the closed first x86 `R_X86_64_PLT32` slice
- retargeting unrelated local helper-call runtime fixtures
- broader symbol-model, GOT/PLT, shared-library, or dynamic-linking work

## Working Model

- treat the staged external-call object contract as the source of truth:
  `R_X86_64_PLT32` at `.text` offset `1` with addend `-4` targeting
  `helper_ext`
- establish one backend-owned fixture that truly leaves the helper symbol
  undefined through assembly
- only after the contract is explicit, extend the built-in assembler parser,
  encoder, and ELF writer for that single relocation shape

## Execution Rules

- inspect the emitted asm and external-assembler object before changing
  implementation
- add or tighten one focused failing object test before code changes
- prefer the smallest parser, encoder, and ELF-writer edits that make the
  single external-call relocation explicit
- stop and split again if the work requires a second relocation family, data
  relocations, or new linker behavior
- keep `plan_todo.md` updated whenever the active slice changes

## Ordered Steps

### Step 1: Capture The External-Call Reference Contract

Goal: pin down one backend-owned asm path that truly emits an undefined helper
call and the exact relocation metadata it must produce.

Primary target: x86 backend fixture selection plus the current assembler/object
comparison seam.

Actions:

- identify or add one bounded x86 backend fixture that lowers to `call
  helper_ext` or an equivalent undefined helper symbol
- assemble that output with the external assembler and record relocation type,
  offset, addend, symbol shape, and section placement
- identify the narrowest existing test seam that can assert this object
  contract

Completion check:

- the plan records one explicit external-call relocation shape and one focused
  test target for it

### Step 2: Tighten Focused Validation

Goal: make the bounded external-call object path fail deterministically until
the built-in assembler supports it.

Primary target: x86 assembler/object tests and any backend adapter seam needed
to exercise the undefined helper call.

Actions:

- add or update one focused test that requires the built-in assembler to emit
  `R_X86_64_PLT32`, offset `1`, addend `-4`, symbol `helper_ext`
- keep the assertion surface narrow: one call shape, one relocation entry, one
  object form

Completion check:

- targeted validation fails for the missing relocation-bearing built-in
  assembler path and clearly describes the contract

### Step 3: Implement The Minimal Assembler Slice

Goal: support the single external-call relocation shape in the built-in x86
assembler.

Primary target: `src/backend/x86/assembler/` and the bounded x86 codegen
handoff.

Actions:

- port only the parser, encoder, and ELF-writer pieces needed for one undefined
  helper-call relocation
- wire the bounded external-call object-emission path through the built-in x86
  assembler
- keep all broader relocation families deferred

Completion check:

- the focused built-in assembler test passes for the single external-call
  relocation case

### Step 4: Prove End-To-End Behavior And Guard Regressions

Goal: show the bounded built-in assembler relocation path works without
regressing the suite.

Primary target: targeted x86 object validation and full-suite regression logs.

Actions:

- run targeted assembler/object validation for the external-call fixture
- confirm the emitted object matches the staged linker expectations
- run the configured regression flow and compare before/after logs

Completion check:

- targeted external-call validation passes through the built-in assembler path
- full-suite results are monotonic with no newly failing tests
