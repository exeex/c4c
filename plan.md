# x86 Extern Global Array Addressing Runbook

Status: Active
Source Idea: ideas/open/19_backend_x86_extern_global_array_addressing_plan.md
Activated from: ideas/open/19_backend_x86_extern_global_array_addressing_plan.md

## Purpose

Promote the next bounded x86 global-addressing slice through the asm backend by making extern global array base-address formation and one indexed element access explicit and test-backed.

## Goal

Make one extern global array element case pass through the x86 asm path with backend-owned RIP-relative base-address materialization plus indexed access.

## Core Rule

Keep this run focused on one extern global array addressing seam. Do not widen into string-pool addressing, pointer-difference lowering, pointer round-tripping, or generic global-pointer arithmetic.

## Read First

- [ideas/open/19_backend_x86_extern_global_array_addressing_plan.md](/workspaces/c4c/ideas/open/19_backend_x86_extern_global_array_addressing_plan.md)
- [prompts/AGENT_PROMPT_EXECUTE_PLAN.md](/workspaces/c4c/prompts/AGENT_PROMPT_EXECUTE_PLAN.md)
- `tests/c/internal/backend_case/extern_global_array.c`
- `tests/backend/backend_lir_adapter_tests.cpp`

## Current Targets

- capture the current LIR and asm-path failure for the smallest extern global array element case
- identify the narrowest backend-owned representation for RIP-relative base-address formation plus one indexed element load
- add or tighten targeted backend adapter coverage before implementation
- promote one extern global array runtime case through `BACKEND_OUTPUT_KIND=asm`

## Non-Goals

- string-pool addressing
- pointer-difference work
- pointer round-tripping
- mixed local/global address lowering
- built-in assembler or linker work

## Working Model

- the next x86 runtime failures now sit on global-addressing seams rather than local stack-slot formation
- the first bounded case should keep the contract explicit: materialize one extern global array base with RIP-relative addressing, then perform one indexed element access without falling back to LLVM text
- if the implementation starts demanding generic pointer arithmetic helpers, stop and split again instead of widening this runbook

## Execution Rules

- update `plan_todo.md` before changing the active slice
- capture current emitted LIR or asm-path failure for the exact extern-global-array case before changing backend code
- add the narrowest backend adapter or contract test before implementation
- keep the first implementation local to RIP-relative base-address formation plus one bounded indexed access
- if execution uncovers a broader global-addressing initiative, record it as a separate idea instead of widening this plan

## Ordered Steps

### Step 1: Capture The Extern-Array Failure

Goal: pin down the current x86 asm-path behavior for the smallest extern global array case.

Primary target:

- `tests/c/internal/backend_case/extern_global_array.c`

Actions:

- rebuild and run the current targeted backend runtime case
- capture the emitted LIR or asm-path failure details for `extern_global_array.c`
- identify the exact global-addressing operations the backend does not yet own
- record the observed failure mode in `plan_todo.md`

Completion check:

- the failing extern-global-array case is reproduced outside the full suite
- the missing RIP-relative base-address plus indexed-load seam is written down concretely

### Step 2: Tighten Backend Validation

Goal: add focused coverage that rejects fallback or missing lowering for this exact extern global array seam.

Primary target:

- `tests/backend/backend_lir_adapter_tests.cpp`

Actions:

- identify the narrowest adapter, contract, or backend test surface for extern global array addressing
- add or tighten one targeted test around RIP-relative base-address materialization plus one indexed element load
- keep the test local to the chosen runtime slice instead of snapshotting unrelated backend output

Completion check:

- one focused test fails before the implementation change
- the test makes the expected global-addressing seam explicit

### Step 3: Implement The Extern-Array Seam

Goal: lower the bounded extern global array case through the x86 asm backend with explicit backend-owned addressing.

Primary target:

- the x86 backend seam responsible for extern global array base-address formation and indexed element access

Actions:

- trace the runtime case to the first backend-owned gap
- implement the smallest RIP-relative base plus indexed-load lowering needed for the target case
- avoid unrelated backend or ABI refactors

Completion check:

- `extern_global_array.c` passes through `BACKEND_OUTPUT_KIND=asm`
- the focused backend test passes without broad unrelated diffs

### Step 4: Validate And Record Follow-On Work

Goal: confirm the slice is monotonic and leave clean execution state.

Actions:

- rerun the targeted backend tests and the promoted runtime case
- rerun the full configured regression suite
- update `plan_todo.md` with completed steps, remaining blockers, and whether the idea is ready to close

Completion check:

- targeted and full-suite validation are recorded
- any broader global-addressing follow-on work is explicitly separated
