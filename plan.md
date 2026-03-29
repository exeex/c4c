# x86 Regalloc And Peephole Enablement Runbook

Status: Active
Source Idea: ideas/open/16_backend_x86_regalloc_peephole_enablement_plan.md
Activated from: ideas/open/16_backend_x86_regalloc_peephole_enablement_plan.md

## Purpose

Turn on the already ported shared liveness/regalloc stack and the first bounded x86-local peephole cleanup path for the x86 backend without expanding into broader backend bring-up, global addressing, assembler, or linker work.

## Goal

Prove one narrow x86 asm slice where shared used-callee-saved information affects the emitted prologue or epilogue and one bounded text-level peephole cleanup removes obviously redundant backend-owned instruction traffic.

## Core Rule

Keep this plan behavior-preserving and narrow. Do not broaden it into general x86 optimization, generic instruction selection changes, assembler work, linker work, or unrelated local/global addressing support.

## Read First

- [ideas/open/16_backend_x86_regalloc_peephole_enablement_plan.md](/workspaces/c4c/ideas/open/16_backend_x86_regalloc_peephole_enablement_plan.md)
- [prompts/AGENT_PROMPT_EXECUTE_PLAN.md](/workspaces/c4c/prompts/AGENT_PROMPT_EXECUTE_PLAN.md)
- `ideas/closed/15_backend_x86_port_plan.md`
- `ideas/closed/03_backend_regalloc_peephole_port_plan.md`

## Current Targets

- thread shared regalloc results into x86 save/restore decisions
- prove one bounded callee-saved-sensitive x86 asm case end to end
- enable only the narrowest post-codegen x86 peephole cleanup needed by that same slice

## Non-Goals

- built-in assembler work
- built-in linker work
- generic x86 optimization or broad cleanup widening
- unrelated local-memory or global-addressing work
- hidden rewrites of instruction selection or LIR contracts

## Working Model

- shared liveness and register allocation are already ported and should remain the source of truth
- x86 integration should attach to existing backend-owned prologue, epilogue, and emit paths
- peephole cleanup must stay as explicit post-codegen text cleanup, not earlier semantic lowering
- one narrow runtime or backend-owned fixture should drive each implementation slice

## Execution Rules

- update `plan_todo.md` before changing the active slice
- add or tighten the narrowest validating test before implementation
- compare against existing x86 output and Clang behavior when ABI or save/restore behavior is unclear
- prefer mechanical integration at existing x86 seams over new shared abstractions
- if broader assembler, linker, or addressing needs appear, record them and stop rather than silently widening this plan

## Ordered Steps

### Step 1: Capture The First Bounded Proof Case

Goal: choose one x86 asm case where callee-saved usage or redundant text cleanup should be observable.

Primary target:

- one direct-call or compare-and-branch x86 backend-owned slice

Actions:

- inspect current x86 backend tests and runtime cases for a minimal function that should exercise used-callee-saved tracking or obvious redundant compare/move/push/pop traffic
- confirm the current emitted x86 assembly shape and where fallback or hardcoded save/restore behavior still exists
- record the chosen proof case and exact target seam in `plan_todo.md`

Completion check:

- one concrete case is selected
- the expected prologue/epilogue or peephole improvement is written down before implementation starts

### Step 2: Wire Shared Regalloc Results Into x86 Save/Restore Decisions

Goal: make the x86 backend consume shared used-callee-saved information instead of backend-local hardcoded behavior.

Primary target:

- x86 prologue, epilogue, and related emit paths that decide save or restore policy

Actions:

- identify the current x86 seam that computes or assumes callee-saved register preservation
- thread shared regalloc output through that seam with the smallest possible interface change
- add or tighten targeted tests that fail if the x86 path falls back to old hardcoded save/restore behavior

Completion check:

- x86 save/restore decisions depend on shared regalloc results
- the selected bounded test proves the changed prologue or epilogue behavior

### Step 3: Enable The Narrowest x86 Peephole Cleanup Needed By The Same Slice

Goal: remove one class of obviously redundant backend-owned text instructions without turning peephole cleanup into a general optimizer.

Primary target:

- x86-local post-codegen text cleanup for redundant compare, move, push, or pop traffic exposed by the chosen proof case

Actions:

- inspect the existing shared or target-local peephole entry boundary
- enable only the smallest cleanup pass required by the selected x86 case
- keep the cleanup after codegen emission and avoid changing semantic lowering
- add or tighten tests that assert the redundant text pattern is removed in the bounded slice

Completion check:

- one bounded x86 peephole cleanup is active
- the selected case shows a concrete emitted-assembly improvement without changing intended behavior

### Step 4: Validate And Stop At The Intended Boundary

Goal: prove the narrow slice works and leave clear state for the next iteration.

Actions:

- run the targeted tests for the chosen x86 seam
- run nearby backend tests that cover x86 asm output, regalloc integration, and peephole behavior
- run the full configured regression suite required by the execute-plan prompt
- update `plan_todo.md` with completed steps, next follow-up slice, and any blockers or deferred work

Completion check:

- targeted and full-suite validation are recorded
- no new unrelated work has been absorbed into the plan
- the next slice is explicit if the plan remains active
