# AArch64 For-Loop Control Runtime Hang Plan

Status: Active
Source Idea: ideas/open/283_aarch64_for_loop_control_runtime_hang.md
Activated After: ideas/closed/276_aarch64_c_testsuite_backend_runtime_execution.md

## Purpose

Repair the next AArch64 backend loop/control lowering failure exposed by the
broader c-testsuite backend route after the route itself was proven ready.

## Goal

Make `tests/c/external/c-testsuite/src/00007.c` stop hanging by lowering the
supported `for`/loop-control shape to truthful AArch64 control flow with real
loop-exit paths.

## Core Rule

Fix semantic loop/control lowering. Do not use filename-specific matching,
c-testsuite shortcuts, timeout or expectation changes, LLVM IR fallback, or a
rewrite of completed scalar local, pointer storage, or `00005.c`/`00006.c`
branch repairs as progress.

## Read First

- `ideas/open/283_aarch64_for_loop_control_runtime_hang.md`
- `todo.md`
- `ideas/closed/276_aarch64_c_testsuite_backend_runtime_execution.md`
- `ideas/closed/282_aarch64_loop_branch_control_runtime_hang.md`
- latest `test_after.log`, if still present, for the `00007.c` hang evidence
- AArch64 backend branch/control lowering, CFG edge selection, comparison
  lowering, label emission, loop handling, and return lowering paths touched by
  `for`/loop-control forms

## Current Targets

- `tests/c/external/c-testsuite/src/00007.c`: for-loop/control lowering that
  currently emits unconditional self-loops.
- Focused backend loop/control proof outside the exact c-testsuite filename
  where practical.
- Regression proof for the green AArch64 backend runtime route through
  `00001.c` to `00006.c`.

## Non-Goals

- Do not change c-testsuite `.expected` sidecars, allowlists, unsupported
  classifications, CTest contracts, runner files, or timeout policy to claim
  progress.
- Do not add filename-specific handling for `00007.c`.
- Do not route proof through LLVM IR fallback while claiming AArch64 backend
  progress.
- Do not reopen completed scalar local, address-exposed pointer storage, or
  `00005.c`/`00006.c` fused-compare branch repairs unless their exact old
  owner-layer failures return.
- Do not perform broad parser, frontend, ABI, register-allocation, or route
  runner rewrites unless trace evidence proves this loop-control owner requires
  them.

## Working Model

The AArch64 backend c-testsuite runtime route is now proven through `00006.c`.
The broader inventory selected 220 tests and blocked at `00007.c` with
`[RUNTIME_HANG]`. Generated `.s` and `.bin` artifacts exist, so the route
reached backend assembly generation and clang assembler/link. The assembly
contains unconditional self-loops before expected loop-exit conditions,
including a loop body that decrements a stack local and branches back
unconditionally.

This is a backend loop/control lowering owner layer, not runtime route
infrastructure.

## Execution Rules

- Keep each packet bounded to investigation, one semantic repair, or one proof
  update.
- Update `todo.md` with the current packet result and exact proof command.
- Preserve `test_after.log` as the canonical executor proof log for routine
  packets unless the supervisor delegates another artifact.
- Prefer AST-backed symbol queries via `c4c-clang-tools` for large C++ backend
  files.
- Use timeout-bounded runtime proof or assembly-only localization while the
  `00007.c` hang remains active.
- Treat a change as route drift if it only makes `00007.c` pass without
  repairing equivalent `for` or loop-control forms.

## Steps

### Step 1: Localize For-Loop Control Ownership

Goal: identify where the `00007.c` `for`/loop-control facts are lost or emitted
as unconditional self-loops.

Actions:

- Inspect the generated `00007.c` assembly and relevant HIR/BIR/MIR or prepared
  control-flow facts.
- Trace how loop conditions and exit edges should become conditional branches.
- Identify whether the owner is comparison lowering, branch target selection,
  CFG edge consumption, loop latch/header emission, label emission, or return
  emission ordering.
- Record the smallest semantic repair point and a timeout-safe proof command in
  `todo.md`.

Completion check:

- `todo.md` names the owner function or backend stage, explains why `00007.c`
  currently self-loops, and gives the exact proof subset for the repair packet.

### Step 2: Repair Supported For-Loop Control Lowering

Goal: lower the supported `for`/loop-control form into truthful AArch64 control
flow.

Actions:

- Implement the smallest semantic backend/codegen change at the owner point
  identified in Step 1.
- Preserve the already-green runtime route for `00001.c` through `00006.c`.
- Preserve completed scalar local, pointer storage, and fused-compare branch
  behavior unless trace evidence proves the exact old owner returned.
- Build the affected backend target before runtime proof.

Completion check:

- `00007.c` no longer hangs due to unconditional self-loops, and focused
  backend loop/control proof covers the repaired rule.

### Step 3: Prove Runtime And Regression Coverage

Goal: prove the loop-control repair and reject testcase overfit.

Actions:

- Run focused backend loop/control coverage.
- Run the AArch64 backend route for `00007.c` with timeout-safe handling.
- Rerun `00001.c` through `00006.c` to prove the prior route remains green.
- Confirm no LLVM IR fallback was used and no expectations, allowlists, runner
  files, unsupported classifications, or timeout policy were weakened.
- Preserve exact proof output in `test_after.log`.

Completion check:

- `00007.c` advances through the owned loop-control failure mode, prior proof
  cases remain green, and any later failure family is classified truthfully.

### Step 4: Review Residual Scope

Goal: decide whether idea 283 is complete or whether later failure families
should be split.

Actions:

- Compare the implementation and proof against
  `ideas/open/283_aarch64_for_loop_control_runtime_hang.md`.
- Confirm no route infrastructure, expectation, runner, timeout, or broad
  frontend/ABI work was folded into this plan.
- If complete, request plan-owner closure review.
- If proof exposes a distinct later issue, create or request a focused
  follow-up idea instead of expanding this plan.

Completion check:

- `todo.md` clearly says whether idea 283 is ready for closure review or names
  the exact blocker/follow-up owner layer.
