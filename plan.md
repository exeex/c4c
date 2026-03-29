# x86 Global Addressing Runbook

Status: Active
Source Idea: ideas/open/18_backend_x86_global_addressing_plan.md

## Purpose

Promote the next bounded x86 asm backend slice beyond exact scalar global loads by making one explicit backend-owned global base-address formation seam work end to end.

## Goal

Implement one narrow x86 global-addressing case through `BACKEND_OUTPUT_KIND=asm` with explicit RIP-relative base-address formation and one indexed-load path.

## Core Rule

Keep this plan limited to one backend-owned global-addressing seam. If the work starts requiring generic pointer arithmetic, mixed local/global lowering, or broader assembler/linker expansion, stop and split a new idea instead of widening this runbook.

## Read First

- [ideas/open/18_backend_x86_global_addressing_plan.md](/workspaces/c4c/ideas/open/18_backend_x86_global_addressing_plan.md)
- [ideas/open/__backend_port_plan.md](/workspaces/c4c/ideas/open/__backend_port_plan.md)
- [prompts/AGENT_PROMPT_EXECUTE_PLAN.md](/workspaces/c4c/prompts/AGENT_PROMPT_EXECUTE_PLAN.md)

## Current Scope

- one bounded extern-global-array element load through the x86 asm path, or
- one bounded string-pool decay/indexed-byte load through the x86 asm path

Choose the smaller slice after inspecting the current backend and test surfaces. Preserve the follow-on pointer-difference and pointer-roundtrip work as separate ideas.

## Non-Goals

- broad local-memory lowering
- generic pointer arithmetic support
- mixed local/global address lowering
- pointer-difference implementation beyond bounded notes needed to preserve future work
- built-in assembler or linker work
- broad relocation-model completeness

## Working Model

- Treat the current exact scalar global-load path as the last completed seam.
- Add the narrowest backend-owned representation needed for RIP-relative base-address formation.
- Tighten adapter or emitter coverage before widening backend logic.
- Promote exactly one runtime case through the asm path once backend tests prove fallback is no longer required.

## Execution Rules

- Prefer the smallest existing runtime case that exposes address formation cleanly.
- Compare against Clang/LLVM output before guessing address materialization behavior.
- Keep tests and implementation aligned to one root cause at a time.
- Record any adjacent but out-of-scope discoveries back into the source idea or a new `ideas/open/*.md` file.

## Ordered Steps

### Step 1: Choose the narrowest promotable case

Goal: select the smallest remaining x86 global-addressing runtime case that requires explicit backend-owned base-address formation.

Primary targets:

- `tests/c/internal/backend_case/string_literal_char.c`
- `tests/c/internal/backend_case/global_int_pointer_diff.c`
- `tests/c/internal/backend_case/global_int_pointer_roundtrip.c`
- any existing synthetic extern-global-array fixture already present in backend tests

Actions:

- inspect current backend tests and runtime cases for the smallest case that is still on fallback
- confirm the chosen case needs RIP-relative base-address formation plus one indexed access
- record the chosen case in `plan_todo.md` before code changes

Completion check:

- one exact case is selected and justified as the smallest in-scope slice

### Step 2: Tighten backend-facing validation first

Goal: make the chosen seam observable in tests before implementation.

Primary targets:

- x86 backend adapter tests
- x86 backend emitter tests

Actions:

- add or tighten a backend test that stops accepting fallback for the chosen global-addressing seam
- keep the assertion narrow to one global base-address formation plus one indexed-load contract
- capture the failing baseline before implementation

Completion check:

- a targeted backend test fails for the chosen seam and clearly distinguishes fallback from explicit x86 handling

### Step 3: Implement the narrow backend-owned address-formation seam

Goal: teach the x86 asm path exactly enough to materialize the chosen global base address and perform the required indexed load.

Primary targets:

- x86 backend lowering, matching, or emission surfaces discovered in Step 1

Actions:

- add the smallest backend representation needed for RIP-relative global base-address formation
- implement one indexed-load path tied to the chosen case
- avoid widening into generic pointer arithmetic or unrelated lowering families

Completion check:

- the targeted backend validation passes without relying on fallback for the selected seam

### Step 4: Promote the runtime case through `BACKEND_OUTPUT_KIND=asm`

Goal: prove the bounded runtime case works through the x86 asm backend path.

Primary targets:

- the chosen runtime case from Step 1

Actions:

- run the selected case through `BACKEND_OUTPUT_KIND=asm`
- compare generated IR and behavior against Clang or existing reference expectations as needed
- fix only issues required for this bounded seam

Completion check:

- the chosen runtime case passes through the asm path and matches the intended bounded behavior

### Step 5: Validate and preserve boundaries

Goal: finish with monotonic regression evidence and clear next-slice notes.

Actions:

- run the targeted tests, nearby subsystem coverage, and the full `ctest` suite
- compare before and after logs for monotonic results
- update `plan_todo.md` with completed work, next intended slice, and any blocked adjacent ideas

Completion check:

- full-suite results are monotonic and the next follow-on work is recorded without expanding this plan
