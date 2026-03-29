# Active Plan: x86 Built-in Assembler Call Relocation

Status: Active
Source Idea: ideas/open/32_backend_builtin_assembler_x86_call_relocation_plan.md

## Purpose

Advance the next bounded x86 backend slice by teaching the built-in assembler
to emit the relocation-bearing object needed for the already-closed helper-call
runtime seam.

## Goal

Make the built-in x86 assembler produce the single external-call relocation
shape required by `tests/c/internal/backend_case/call_helper.c`, then prove the
existing bounded linker path still closes the case correctly.

## Core Rule

Keep this slice strictly limited to one relocation-bearing helper-call object
path. Do not widen into generic relocation coverage, new linker features, or
broader assembler parity work.

## Read First

- `ideas/open/32_backend_builtin_assembler_x86_call_relocation_plan.md`
- `ideas/open/__backend_port_plan.md`
- `ideas/closed/23_backend_builtin_assembler_x86_plan.md`
- `ideas/closed/24_backend_builtin_linker_x86_plan.md`
- `ideas/closed/31_backend_x86_runtime_case_convergence_plan.md`
- `prompts/AGENT_PROMPT_EXECUTE_PLAN.md`

## Current Targets / Scope

- `src/backend/x86/assembler/`
- `src/backend/x86/codegen/`
- `src/backend/x86/codegen/emit.hpp`
- x86 backend adapter / assembler object tests covering relocation-bearing
  helper calls
- `tests/c/internal/backend_case/call_helper.c`

## Non-Goals

- generic x86 relocation completeness
- data-address or RIP-relative global relocation work beyond this helper-call
  path
- dynamic linking, shared libraries, TLS, GOT/PLT expansion, or new linker
  mechanisms
- unrelated x86 runtime-case convergence work already closed in prior child
  ideas

## Working Model

- treat the external assembler output for the bounded helper-call case as the
  reference object contract
- reuse the already-closed first linker relocation path instead of extending
  linker scope
- keep validation centered on one relocation-bearing object shape and one
  end-to-end helper-call case

## Execution Rules

- inspect the emitted asm and object contract before changing implementation
- add or tighten the narrowest failing test before code changes
- prefer the smallest parser/encoder/object-writer edits that make the bounded
  relocation path explicit
- stop and split into a new idea if the work requires a second relocation
  family, broader symbol model changes, or new linker behavior
- keep `plan_todo.md` updated whenever the active slice changes

## Ordered Steps

### Step 1: Capture The Bounded Reference Contract

Goal: pin down the exact helper-call asm and relocation metadata the built-in
assembler must match.

Primary target: `tests/c/internal/backend_case/call_helper.c` and the current
x86 backend assembly/object comparison path.

Actions:

- inspect the current backend-owned asm emitted for the x86 helper-call case
- assemble that output with the external assembler and record the relocation
  type, symbol shape, and section placement
- identify the narrowest existing test seam that can assert this object
  contract

Completion check:

- the plan records one explicit relocation shape and one focused test target for
  the helper-call object path

### Step 2: Tighten Focused Validation

Goal: make the bounded relocation-bearing helper-call object path fail
deterministically until the built-in assembler supports it.

Primary target: x86 assembler/object tests and any helper-call adapter seam
tests.

Actions:

- add or update one focused test that requires the built-in assembler to emit
  the expected external-call relocation
- keep the assertion surface narrow: one call shape, one relocation entry, one
  object form

Completion check:

- targeted validation fails for the missing relocation-bearing built-in
  assembler path and clearly describes the contract

### Step 3: Implement The Minimal Assembler Slice

Goal: support the single relocation-bearing helper-call object shape in the
built-in x86 assembler.

Primary target: `src/backend/x86/assembler/` and the bounded x86 codegen
handoff.

Actions:

- port only the parser, encoder, and ELF-writer pieces needed for one external
  helper-call relocation
- wire the bounded helper-call object-emission path through the built-in x86
  assembler
- keep all broader relocation families deferred

Completion check:

- the focused built-in assembler test passes for the helper-call relocation
  case

### Step 4: Prove End-To-End Behavior And Guard Regressions

Goal: show the built-in assembler path works for the bounded helper-call case
without regressing the suite.

Primary target: `tests/c/internal/backend_case/call_helper.c` and full-suite
regression logs.

Actions:

- run targeted helper-call runtime and assembler/object validation
- confirm the linked helper-call result matches the existing reference path
- run the full configured regression flow and compare before/after logs

Completion check:

- targeted helper-call validation passes through the built-in assembler path
- full-suite results are monotonic with no newly failing tests
