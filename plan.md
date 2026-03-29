# x86 Runtime Case Convergence Runbook

Status: Active
Source Idea: ideas/open/31_backend_x86_runtime_case_convergence_plan.md
Activated from: ideas/open/31_backend_x86_runtime_case_convergence_plan.md

## Purpose

Turn a bounded set of real x86 runtime cases from LLVM-text fallback into stable backend-owned asm execution, one testcase family at a time.

## Goal

Make promoted x86 backend cases emit assembly text and pass end-to-end runtime validation with `BACKEND_OUTPUT_KIND=asm`, starting with `tests/c/internal/backend_case/call_helper.c`.

## Core Rule

Promote one narrow runtime testcase family at a time. Treat LLVM-text fallback as a bug for any case explicitly in scope, and do not widen into general x86 assembler/linker or ABI-completeness work.

## Read First

- [ideas/open/31_backend_x86_runtime_case_convergence_plan.md](/workspaces/c4c/ideas/open/31_backend_x86_runtime_case_convergence_plan.md)
- [ideas/open/__backend_port_plan.md](/workspaces/c4c/ideas/open/__backend_port_plan.md)
- [prompts/AGENT_PROMPT_EXECUTE_PLAN.md](/workspaces/c4c/prompts/AGENT_PROMPT_EXECUTE_PLAN.md)

## Current Targets / Scope

- Primary first slice: `tests/c/internal/backend_case/call_helper.c`
- Immediate follow-on family after helper calls:
  - `param_slot`
  - `two_arg_helper`
  - `two_arg_local_arg`
  - `two_arg_second_local_arg`
- Later bounded families, only after helper-call and parameter seams are explicit:
  - compare-and-branch runtime cases
  - adjacent local-slot / local-address cases
  - the smallest backend-owned global-addressing cases

## Non-Goals

- full x86 assembler completeness
- full x86 linker completeness
- broad x86 optimization cleanup
- broad pointer arithmetic or ABI-completeness work
- silently absorbing a new mechanism family without splitting a follow-on idea

## Working Model

- Real runtime `.c` cases are the proof target.
- Each promoted runtime case should gain the narrowest backend adapter / emitter assertion that proves the exact seam.
- Validation must include at least one real emitted `.s -> binary -> exit code` path for the active testcase family.
- If a new testcase family requires materially different lowering or addressing machinery, stop and stage a separate idea instead of mutating this runbook.

## Execution Rules

- Follow `plan_todo.md` as the mutable execution state for the current slice.
- Use test-first slices: add or tighten the narrowest test before changing implementation.
- Compare behavior against the current backend-owned asm path and against Clang/LLVM where needed.
- Keep target-specific changes isolated to the x86 backend seam under active investigation.
- Do not weaken checks to tolerate LLVM fallback for in-scope promoted cases.

## Ordered Steps

### Step 1: Establish the helper-call baseline

Goal: prove the current `call_helper.c` x86 behavior and identify the exact fallback seam.

Primary target:
- `tests/c/internal/backend_case/call_helper.c`

Concrete actions:
- record the focused failure mode when `BACKEND_OUTPUT_KIND=asm` is requested
- inspect the backend adapter / routing path that still emits LLVM text for this case
- capture the narrowest emitter or adapter seam that must change for helper calls to stay on the backend-owned asm path

Completion check:
- the exact current failure is reproduced and localized to a bounded x86 backend seam

### Step 2: Promote `call_helper.c` onto the backend-owned asm path

Goal: make the first helper-call runtime case produce backend-owned x86 assembly instead of LLVM text.

Primary target:
- `tests/c/internal/backend_case/call_helper.c`

Concrete actions:
- add or tighten the narrowest backend adapter / emitter assertion for the helper-call seam
- implement the smallest x86 backend change needed to keep this case on the asm path end to end
- verify the compiler output for the promoted case is assembly text, not LLVM IR

Completion check:
- `call_helper.c` emits backend-owned x86 assembly and passes its focused runtime validation

### Step 3: Promote the narrow parameter-passing family

Goal: extend the same backend-owned asm path to the smallest adjacent argument-lowering cases.

Primary targets:
- `param_slot`
- `two_arg_helper`
- `two_arg_local_arg`
- `two_arg_second_local_arg`

Concrete actions:
- add or update the narrowest per-case assertions that prove argument lowering stays on the x86 asm path
- implement only the argument-passing machinery required by these cases
- keep helper-call behavior stable while expanding to one parameter slice at a time

Completion check:
- the narrow parameter family in scope passes targeted backend and runtime checks with `BACKEND_OUTPUT_KIND=asm`

### Step 4: Tighten bounded compare / local / global follow-ons

Goal: extend coverage only to the smallest adjacent runtime seams once helper-call and parameter slices are stable.

Concrete actions:
- promote the smallest compare-and-branch runtime cases first
- then promote local-slot / local-address cases that already sit close to the existing x86 backend seam
- only after that, promote the smallest global-addressing cases that do not require broad new lowering machinery

Completion check:
- each newly promoted family stays on backend-owned asm with a matching targeted assertion and runtime proof

### Step 5: Guard the slice and decide on follow-on planning

Goal: finish the active bounded family without accidentally widening the initiative.

Concrete actions:
- re-run targeted tests and full-suite validation after each meaningful slice
- record any newly discovered separate mechanism family under `ideas/open/` instead of extending this runbook
- keep `plan_todo.md` current with completed slices, next slice, blockers, and resume notes

Completion check:
- the active promoted family is stable, validated, and any out-of-scope next work is explicitly staged elsewhere

## Validation

- `backend_lir_adapter_tests` must stop accepting LLVM fallback for each promoted x86 slice
- targeted runtime tests under `tests/c/internal/backend_case/` must pass with `BACKEND_OUTPUT_KIND=asm`
- compiler output for promoted x86 cases must be assembly text, not LLVM IR
- compile/run verification must include at least one real emitted `.s -> binary -> exit code` path for the active testcase family
- full `ctest` results must remain monotonic for completed implementation slices
