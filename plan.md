# x86 Local Memory Addressing Runbook

Status: Active
Source Idea: ideas/open/17_backend_x86_local_memory_addressing_plan.md
Activated from: ideas/open/17_backend_x86_local_memory_addressing_plan.md

## Purpose

Promote the first bounded x86 local-memory slice through the asm backend by making local stack-slot addressing explicit and test-backed instead of falling back or relying on ad hoc lowering.

## Goal

Make one small local-array runtime case pass through the x86 asm path with a backend-owned stack-base plus indexed-address lowering seam.

## Core Rule

Keep this run focused on one explicit local-memory addressing seam. Do not widen into general pointer arithmetic, global-address formation, or unrelated assembler/linker work.

## Read First

- [ideas/open/17_backend_x86_local_memory_addressing_plan.md](/workspaces/c4c/ideas/open/17_backend_x86_local_memory_addressing_plan.md)
- [prompts/AGENT_PROMPT_EXECUTE_PLAN.md](/workspaces/c4c/prompts/AGENT_PROMPT_EXECUTE_PLAN.md)
- `tests/c/internal/backend_case/local_array.c`
- `tests/backend/backend_lir_adapter_tests.cpp`

## Current Targets

- capture the current LIR and asm-path failure for `tests/c/internal/backend_case/local_array.c`
- identify the narrowest backend-owned representation for stack-local base-address plus indexed local access
- add or tighten targeted backend adapter coverage before implementation
- promote one runtime local-array case through `BACKEND_OUTPUT_KIND=asm`

## Non-Goals

- global-address materialization
- general pointer arithmetic lowering
- aggregate or member-addressing expansion beyond what the first local-array slice needs
- built-in assembler or linker work

## Working Model

- the remaining failing x86 runtime cases already show local-memory addressing is a distinct seam from the closed struct-return function-pointer fix
- `local_array.c` is the narrowest current runtime target because it requires explicit stack-local address formation without immediately demanding mixed global/local behavior
- the backend contract should stay explicit: recognize or carry one stack-base plus indexed element access shape instead of collapsing the work into generic fallback logic

## Execution Rules

- update `plan_todo.md` before changing the active slice
- capture current LIR or emitted asm for the exact runtime case before changing backend code
- add the narrowest backend adapter or contract test before implementation
- keep the first implementation local to x86 stack-slot plus indexed-address lowering
- if execution uncovers a broader pointer or aggregate initiative, record it as a separate idea instead of widening this plan

## Ordered Steps

### Step 1: Capture The Local-Array Failure

Goal: pin down the current x86 asm-path behavior for the smallest local-memory case.

Primary target:

- `tests/c/internal/backend_case/local_array.c`

Actions:

- rebuild and run the current targeted backend runtime case
- capture the emitted LIR or asm-path failure details for `local_array.c`
- identify the exact local-addressing operations the backend does not yet own
- record the observed failure mode in `plan_todo.md`

Completion check:

- the failing local-array case is reproduced outside the full suite
- the missing stack-base/indexed-address seam is written down concretely

### Step 2: Tighten Backend Validation

Goal: add focused coverage that rejects fallback or missing lowering for this exact local-memory seam.

Primary target:

- `tests/backend/backend_lir_adapter_tests.cpp`

Actions:

- identify the narrowest adapter, contract, or backend test surface for local stack-slot addressing
- add or tighten one targeted test around stack-base materialization plus indexed local access
- keep the test local to the chosen runtime slice instead of snapshotting unrelated backend output

Completion check:

- one focused test fails before the implementation change
- the test makes the expected local-memory seam explicit

### Step 3: Implement The Local-Memory Seam

Goal: lower the bounded local-array case through the x86 asm backend with explicit backend-owned addressing.

Primary target:

- the x86 backend seam responsible for local stack-slot addressing and indexed local access

Actions:

- trace the local-array runtime case to the first backend-owned gap
- implement the smallest stack-base plus indexed-address lowering needed for the target case
- avoid unrelated backend or ABI refactors

Completion check:

- `local_array.c` passes through `BACKEND_OUTPUT_KIND=asm`
- the focused backend test passes without broad unrelated diffs

### Step 4: Validate And Record Follow-On Work

Goal: confirm the slice is monotonic and leave clean execution state.

Actions:

- rerun the targeted backend tests and the promoted runtime case
- rerun the full configured regression suite
- update `plan_todo.md` with completed steps, remaining blockers, and whether the idea is ready to close

Completion check:

- targeted and full-suite validation are recorded
- any broader local-memory or pointer-lowering follow-on work is explicitly separated
