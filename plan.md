# x86 Global Char Pointer Diff Runbook

Status: Active
Source Idea: ideas/open/20_backend_x86_global_char_pointer_diff_plan.md
Activated from: ideas/open/20_backend_x86_global_char_pointer_diff_plan.md

## Purpose

Promote the next bounded x86 global-addressing seam through the asm backend by making one global `char*` pointer-difference path explicit and test-backed.

## Goal

Make `tests/c/internal/backend_case/global_char_pointer_diff.c` pass through the x86 asm path with backend-owned global base-address formation and bounded pointer subtraction semantics.

## Core Rule

Keep this run focused on one global `char*` pointer-difference seam. Do not widen into integer-element scaling, pointer round-tripping, string-pool addressing, or generic global-pointer arithmetic.

## Read First

- [ideas/open/20_backend_x86_global_char_pointer_diff_plan.md](/workspaces/c4c/ideas/open/20_backend_x86_global_char_pointer_diff_plan.md)
- [prompts/AGENT_PROMPT_EXECUTE_PLAN.md](/workspaces/c4c/prompts/AGENT_PROMPT_EXECUTE_PLAN.md)
- `tests/c/internal/backend_case/global_char_pointer_diff.c`
- `tests/backend/backend_lir_adapter_tests.cpp`

## Current Targets

- capture the current LIR and asm-path failure for the bounded x86 global `char*` pointer-difference case
- identify the narrowest backend-owned representation for global base-address formation plus byte-pointer subtraction
- add or tighten targeted backend adapter coverage before implementation
- promote the runtime case through `BACKEND_OUTPUT_KIND=asm`

## Non-Goals

- integer-element pointer subtraction
- pointer round-tripping
- string-pool addressing
- mixed local/global address lowering
- built-in assembler or linker work

## Working Model

- the remaining x86 runtime failures now include broader global-addressing seams beyond the completed extern-global-array slice
- `global_char_pointer_diff.c` is the narrowest next slice because it keeps pointer arithmetic at byte granularity without integer-element scaling
- if the implementation starts demanding generic pointer materialization helpers, stop and split again instead of widening this runbook

## Execution Rules

- update `plan_todo.md` before changing the active slice
- capture the current emitted LIR or asm-path failure for the exact `global_char_pointer_diff.c` case before changing backend code
- add the narrowest backend adapter or contract test before implementation
- keep the first implementation local to global base-address formation plus byte-pointer subtraction
- if execution uncovers a broader global-addressing initiative, record it as a separate idea instead of widening this plan

## Ordered Steps

### Step 1: Capture The Global Char Pointer-Diff Failure

Goal: pin down the current x86 asm-path behavior for the bounded global `char*` pointer-difference case.

Primary target:

- `tests/c/internal/backend_case/global_char_pointer_diff.c`

Actions:

- rebuild and run the current targeted backend runtime case
- capture the emitted LIR or asm-path failure details for `global_char_pointer_diff.c`
- identify the exact global-addressing and subtraction operations the backend does not yet own
- record the observed failure mode in `plan_todo.md`

Completion check:

- the failing runtime case is reproduced outside the full suite
- the missing base-address plus byte-pointer-difference seam is written down concretely

### Step 2: Tighten Backend Validation

Goal: add focused coverage that rejects fallback or missing lowering for this exact x86 global `char*` pointer-difference seam.

Primary target:

- `tests/backend/backend_lir_adapter_tests.cpp`

Actions:

- identify the narrowest adapter, contract, or backend test surface for the global `char*` pointer-difference case
- add or tighten one targeted test around global base-address formation plus byte-pointer subtraction
- keep the test local to the chosen runtime slice instead of snapshotting unrelated backend output

Completion check:

- one focused test fails before the implementation change
- the test makes the expected pointer-difference seam explicit

### Step 3: Implement The Global Char Pointer-Diff Seam

Goal: lower the bounded global `char*` pointer-difference case through the x86 asm backend with explicit backend-owned addressing and subtraction.

Primary target:

- the x86 backend seam responsible for global base-address formation and bounded pointer subtraction

Actions:

- trace the runtime case to the first backend-owned gap
- implement the smallest global base plus byte-pointer subtraction lowering needed for the target case
- avoid unrelated backend or ABI refactors

Completion check:

- `global_char_pointer_diff.c` passes through `BACKEND_OUTPUT_KIND=asm`
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
