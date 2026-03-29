# Backend Regalloc And Peephole Port Runbook

Status: Active
Source Idea: ideas/open/03_backend_regalloc_peephole_port_plan.md

## Purpose

Turn the shared backend liveness, register-allocation, stack-layout helper, and minimal late-cleanup layers into an executable C++ port that the AArch64 backend can compile against and begin consuming.

## Goal

Make the smallest ref-shaped shared backend slice compile cleanly and connect to the AArch64 path before tightening behavior.

## Core Rule

Keep this runbook narrow and mechanical: port the shared backend layers in ref-shaped boundaries, validate each slice with targeted tests, and do not silently absorb broader backend, assembler, or linker work into this plan.

## Read First

- [ideas/open/03_backend_regalloc_peephole_port_plan.md](/Users/chi-shengwu/c4c/ideas/open/03_backend_regalloc_peephole_port_plan.md)
- [prompts/AGENT_PROMPT_EXECUTE_PLAN.md](/Users/chi-shengwu/c4c/prompts/AGENT_PROMPT_EXECUTE_PLAN.md)
- `ref/claudes-c-compiler/src/backend/liveness.rs`
- `ref/claudes-c-compiler/src/backend/regalloc.rs`
- `ref/claudes-c-compiler/src/backend/generation.rs`
- `ref/claudes-c-compiler/src/backend/state.rs`
- `ref/claudes-c-compiler/src/backend/stack_layout/regalloc_helpers.rs`
- `ref/claudes-c-compiler/src/backend/stack_layout/analysis.rs`
- `ref/claudes-c-compiler/src/backend/stack_layout/slot_assignment.rs`
- `ref/claudes-c-compiler/src/backend/peephole_common.rs`

## Current Target

- Shared backend compile integration for liveness, interval computation, register allocation, stack-layout helpers, and the smallest AArch64-consumed result boundary.
- AArch64 is the first consumer.
- x86-64 and rv64 should remain future consumers, not active implementation targets in this runbook.

## Non-Goals

- Broad optimizer redesign or SSA work.
- Built-in assembler or linker work.
- Unrelated target-generic backend refactors.
- Full x86-64 or rv64 integration.
- Broad peephole expansion beyond the minimum correctness or practical cleanup needed by the AArch64 path.

## Working Model

- Treat ref as the structure source and C++ port target.
- Keep shared backend logic in shared backend files, not inside AArch64-only code.
- Preserve explicit handoff boundaries between liveness, regalloc, stack-layout helpers, and target-local cleanup.
- Use temporary shims only when they preserve the intended long-term file and interface boundaries.

## Execution Rules

- Start each slice from the highest-priority incomplete item in `plan_todo.md`.
- Add or tighten the narrowest validating test before implementation.
- Keep patches small enough that one root cause and one validation story are visible.
- If execution uncovers a separate initiative, record it under `ideas/open/` instead of expanding this runbook.
- Keep `plan_todo.md` current enough that the next execution turn can resume without re-deriving intent.

## Ordered Steps

### Step 1: Inventory The Shared Backend Boundary

Goal: identify the current C++ shared-backend surfaces, the missing ref-shaped pieces, and the first compile blockers for shared liveness/regalloc integration.

Primary targets:
- `src/backend/`
- `src/backend/stack_layout/`
- AArch64 backend include points that should consume shared regalloc results

Actions:
- inspect the current shared backend tree and the mirrored ref modules
- map which types and helper seams already exist versus which are still scaffolding
- identify the minimum compile slice that can introduce shared liveness and regalloc result types without behavior expansion

Completion check:
- a short, concrete inventory exists in `plan_todo.md` naming the first file slice to port and the first tests to tighten

### Step 2: Port Liveness And Interval Computation

Goal: port the shared liveness and interval-shape layer closely enough that representative multi-block and loop-heavy functions can exercise the ref-shaped data flow.

Primary targets:
- shared liveness files under `src/backend/`
- tests covering interval and liveness shape

Actions:
- add or update targeted tests for interval construction and backward-dataflow liveness
- port the liveness computation and interval-building pieces from ref with minimal structural deviation
- keep value-eligibility and interval metadata explicit for later regalloc use

Completion check:
- shared liveness code compiles
- targeted tests cover representative interval/liveness shapes
- the resulting interfaces are usable by the next regalloc slice

### Step 3: Port Shared Linear-Scan Regalloc

Goal: introduce the shared allocator with the same essential pool split and spill heuristics the ref backend uses.

Primary targets:
- shared regalloc files under `src/backend/`
- tests for register assignment and spill decisions

Actions:
- add or tighten tests for caller-saved versus callee-saved pool selection, call-spanning intervals, spill behavior, and loop-depth-weighted use counts
- port the shared allocator logic and result shape from ref
- preserve explicit used-register tracking needed by target prologue and emit code

Completion check:
- shared regalloc code compiles
- targeted tests validate the ref-shaped assignment behavior for the supported slice
- the port produces a stable `RegAllocResult`-style handoff for stack-layout and AArch64 consumers

### Step 4: Port Stack-Layout Helper Boundary

Goal: make frame-sizing and slot-assignment logic consume shared liveness and register-allocation results through a ref-shaped helper seam.

Primary targets:
- `src/backend/stack_layout/`
- shared result types consumed by stack-layout analysis

Actions:
- port the minimum helper boundary from `regalloc_helpers.rs`, `analysis.rs`, and `slot_assignment.rs`
- thread cached liveness and assigned physical-register data into the frame-sizing path
- keep temporary shims narrow and aligned with the final shared interface

Completion check:
- stack-layout helpers compile against the shared liveness/regalloc results
- targeted tests validate that frame-related decisions can consume the new shared data without ad hoc target-local duplication

### Step 5: Wire The Shared Result Into AArch64

Goal: connect the shared backend result boundary to AArch64 prologue and emit integration points without broadening scope beyond the first consumer seam.

Primary targets:
- AArch64 prologue and emit integration surfaces
- tests that cover used callee-saved register tracking

Actions:
- identify the narrowest AArch64 integration points that should consume shared assigned-register and used-register data
- thread the shared result into those surfaces
- validate that save/restore decisions can be driven by actual used callee-saved sets rather than hardcoded assumptions

Completion check:
- AArch64 backend files can include and reference the shared interfaces
- targeted tests show the new shared result boundary affects the intended integration seam

### Step 6: Add The Smallest Required Cleanup Slice

Goal: port only the minimum late cleanup or peephole behavior required for correctness or obvious quality issues exposed by the new shared path.

Primary targets:
- shared cleanup or peephole support
- AArch64-local cleanup hooks only when strictly required

Actions:
- identify the smallest redundant-stack-traffic or equivalent cleanup case made visible by the new port
- add a narrow test for that case
- port only the minimum cleanup logic needed for the supported AArch64 slice

Completion check:
- one bounded cleanup seam is test-backed and implemented
- the runbook remains narrower than a broad post-codegen optimizer effort

### Step 7: Validate And Stage Follow-On Work

Goal: confirm compile integration and behavior tightening for the active slice, then record the next bounded follow-on.

Actions:
- run targeted tests after each slice and full regression checks before handoff
- compare new failures or uncovered work against the current runbook scope
- record deferred follow-ons in `plan_todo.md` or `ideas/open/` when they are separate initiatives

Completion check:
- full-suite regression status is recorded for the active slice
- `plan_todo.md` reflects completed work, the next intended slice, and any blockers or follow-ons
