# Struct Return Function-Pointer IR Validation Runbook

Status: Active
Source Idea: ideas/open/25_frontend_struct_return_function_pointer_ir_plan.md
Activated from: ideas/open/25_frontend_struct_return_function_pointer_ir_plan.md

## Purpose

Fix the narrow LLVM IR mismatch for indirect struct-return function-pointer calls without widening into general struct ABI or unrelated x86 backend work.

## Goal

Make the emitted LLVM IR for the `struct-ret-1.c` indirect call match the callable type and byval argument ABI shape that Clang accepts on the host triple.

## Core Rule

Keep this run focused on one reproducer: the struct-return indirect-call type mismatch in `llvm_gcc_c_torture_src_struct_ret_1_c`. Do not broaden into general function-pointer cleanup, unrelated struct ABI work, or backend assembler/linker changes.

## Read First

- [ideas/open/25_frontend_struct_return_function_pointer_ir_plan.md](/workspaces/c4c/ideas/open/25_frontend_struct_return_function_pointer_ir_plan.md)
- [prompts/AGENT_PROMPT_EXECUTE_PLAN.md](/workspaces/c4c/prompts/AGENT_PROMPT_EXECUTE_PLAN.md)
- `tests/c/external/gcc_torture/src/struct-ret-1.c`

## Current Targets

- reproduce the invalid indirect-call LLVM IR on the current host triple
- identify the lowering seam that diverges between direct and indirect struct-return calls
- tighten the narrowest test coverage around the callable type and byval argument ABI shape
- fix only the mismatched IR construction needed for the reproducer

## Non-Goals

- unrelated x86 compare-and-branch or regalloc work
- general struct ABI cleanup outside the reproducer
- broad function-pointer lowering redesign
- built-in assembler or linker work

## Working Model

- the failing case already proves direct struct-return calls can carry the expected byval pointer shape
- the bug is likely in indirect-call lowering, where the callee type or argument types drift from the direct-call form
- Clang IR for the same source is the behavioral reference for both the indirect callable type and the call-site argument annotations

## Execution Rules

- update `plan_todo.md` before changing the active slice
- add or tighten the narrowest reproducer test before implementation
- capture both `build/c4cll` output and Clang `-S -emit-llvm -O0` output before changing lowering
- prefer a localized fix at the call-lowering seam over changing unrelated type construction or ABI helpers
- if execution reveals a broader struct ABI initiative, record it as a separate idea instead of widening this plan

## Ordered Steps

### Step 1: Capture The Exact Reproducer

Goal: pin down the current invalid IR and the exact mismatch against Clang.

Primary target:

- `tests/c/external/gcc_torture/src/struct-ret-1.c`

Actions:

- rebuild the tree and reproduce the failing case with `build/c4cll`
- capture the emitted LLVM IR for the failing indirect call
- capture the matching Clang LLVM IR on the host triple
- record the exact callable-type and argument-shape mismatch in `plan_todo.md`

Completion check:

- the failing indirect call is reproduced outside the full suite
- the direct-versus-indirect type mismatch is written down concretely

### Step 2: Tighten The Narrowest Validation

Goal: add focused coverage that fails specifically for this mismatch before the fix lands.

Primary target:

- the smallest backend/frontend validation surface that can assert the indirect-call LLVM IR shape

Actions:

- identify the narrowest existing test harness for emitted LLVM IR around indirect calls or ABI-sensitive calls
- add or tighten one reproducer test that rejects the current invalid indirect-call form
- keep the test targeted at callable type and byval argument shape rather than broad output snapshots

Completion check:

- one focused test fails before the implementation change
- the test describes the expected indirect struct-return call shape clearly

### Step 3: Fix Indirect Struct-Return Call Lowering

Goal: make the indirect call use the same callable type and ABI-relevant argument shape as the accepted direct-call path.

Primary target:

- the function-pointer call lowering seam that builds the callee type and indirect call operands

Actions:

- trace the direct and indirect struct-return call paths to the first point where their type construction diverges
- implement the smallest fix that preserves the existing direct-call behavior
- avoid unrelated ABI or type-system refactors

Completion check:

- the reproducer emits LLVM IR that Clang accepts
- the focused test passes without broad unrelated diffs

### Step 4: Validate And Record Follow-On Work

Goal: prove the fix is monotonic and leave clean execution state.

Actions:

- rerun the focused reproducer and nearby call-lowering tests
- rerun the full configured regression suite
- update `plan_todo.md` with completed steps, any remaining blockers, and whether this idea is ready to close

Completion check:

- targeted and full-suite validation are recorded
- any broader struct ABI follow-on work is explicitly separated
