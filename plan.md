# AArch64 Loop Branch Control Runtime Hang Plan

Status: Active
Source Idea: ideas/open/282_aarch64_loop_branch_control_runtime_hang.md
Activated After: ideas/closed/281_aarch64_address_exposed_local_pointer_runtime_nonzero.md

## Purpose

Repair the AArch64 backend conditional branch and loop-control lowering failure
family exposed after local operand materialization and address-exposed pointer
storage were fixed.

## Goal

Make the supported conditional branch and loop-control forms lower to truthful
AArch64 control flow so `00005.c` no longer returns through the wrong
straight-line first-`if` path and `00006.c` no longer hangs in an unconditional
self-loop.

## Core Rule

Fix semantic branch/control lowering. Do not use filename-specific matching,
c-testsuite shortcuts, timeout or expectation changes, LLVM IR fallback, or a
rewrite of completed address/pointer storage work as progress.

## Read First

- `ideas/open/282_aarch64_loop_branch_control_runtime_hang.md`
- `todo.md`
- `ideas/closed/281_aarch64_address_exposed_local_pointer_runtime_nonzero.md`
- `ideas/closed/278_aarch64_backend_local_operand_materialization_runtime_nonzero.md`
- `test_after.log`, if still present, for latest focused route evidence
- AArch64 backend branch/control lowering, CFG edge selection, comparison
  lowering, label emission, and return lowering paths touched by conditional
  branches or loops

## Current Targets

- `tests/c/external/c-testsuite/src/00005.c`: first-`if` conditional branch
  lowering after pointer storage is already coherent.
- `tests/c/external/c-testsuite/src/00006.c`: loop-control lowering that
  currently emits an unconditional self-loop.
- Focused backend branch/control proof outside exact c-testsuite filenames
  where practical.
- Regression proof for ideas 278 and 281: `00001.c`, `00002.c`, `00003.c`,
  and `00004.c`.

## Non-Goals

- Do not reopen address-exposed local or pointer storage semantics for
  `00004.c`/`00005.c` unless trace evidence proves the exact owner returned.
- Do not change c-testsuite `.expected` sidecars, allowlists, unsupported
  classifications, CTest contracts, runner files, or timeout policy to claim
  progress.
- Do not add filename-specific handling for `00005.c` or `00006.c`.
- Do not route proof through LLVM IR fallback while claiming AArch64 backend
  progress.
- Do not perform broad CFG, parser, frontend, ABI, or register-allocation
  rewrites unless the trace proves the branch-control owner requires them.

## Working Model

After ideas 278 and 281, scalar locals, pointer slot stores, and the `00004.c`
return-handoff path are repaired. The remaining `00005.c` failure is
straight-line branch/control lowering: generated assembly still returns through
`mov w0, #1; ret` for the first `if` path instead of branching over it when the
loaded condition is false.

The `00006.c` failure is another control-flow owner: generated assembly
decrements a stack local but emits an unconditional branch back to the loop
label, leaving the post-loop return unreachable.

## Execution Rules

- Keep each packet bounded to investigation, one semantic repair, or one proof
  update.
- Update `todo.md` with the current packet result and exact proof command.
- Preserve `test_after.log` as the canonical executor proof log for routine
  packets unless the supervisor delegates another artifact.
- Prefer AST-backed symbol queries via `c4c-clang-tools` for large C++ backend
  files.
- Use timeout-bounded runtime proof or assembly-only localization while the
  `00006.c` hang remains active.
- Treat a change as route drift if it only makes one c-testsuite filename pass
  without repairing conditional branch or loop-control lowering.

## Steps

### Step 1: Localize Conditional Branch Ownership

Goal: identify where the `00005.c` first-`if` condition and branch target are
lost or emitted as straight-line return control.

Actions:

- Inspect the generated `00005.c` assembly and relevant HIR/BIR/MIR or prepared
  control-flow facts.
- Trace how the loaded condition should become a conditional branch over the
  wrong return path.
- Identify whether the owner is comparison lowering, branch target selection,
  CFG edge consumption, label emission, or return emission ordering.
- Record the smallest semantic repair point and a focused proof command in
  `todo.md`.

Completion check:

- `todo.md` names the owner function or backend stage, explains why `00005.c`
  currently falls through to the wrong return, and gives the exact proof subset
  for the repair packet.

### Step 2: Repair Supported Conditional Branch Lowering

Goal: lower the supported first-`if` branch form into truthful AArch64 control
flow.

Actions:

- Implement the smallest semantic backend/codegen change at the owner point
  identified in Step 1.
- Preserve address-exposed pointer storage from idea 281 and scalar local
  materialization from idea 278.
- Preserve result-register behavior from idea 277.
- Build the affected backend target before runtime proof.

Completion check:

- `00005.c` no longer returns through the wrong straight-line first-`if` path,
  and focused backend branch/control proof covers the repaired rule.

### Step 3: Localize And Repair Loop Control

Goal: address the `00006.c` hang caused by unconditional loop self-branching.

Actions:

- Inspect `00006.c` generated assembly and control-flow facts with timeout-safe
  proof handling.
- Trace where the loop condition or exit edge is lost.
- Repair the supported loop-control lowering form without weakening runtime
  contracts or timeout policy.
- Keep this step separate from unrelated branch/CFG rewrites unless trace
  evidence proves they share the same owner.

Completion check:

- `00006.c` no longer hangs due to an unconditional self-loop and generated
  control flow contains a truthful loop-exit path.

### Step 4: Prove Runtime And Regression Coverage

Goal: prove the branch/control repairs and reject testcase overfit.

Actions:

- Run focused backend branch/control coverage.
- Run the AArch64 backend route for `00005.c` and `00006.c`.
- Rerun `00001.c`, `00002.c`, `00003.c`, and `00004.c` to prove ideas 278 and
  281 remain green.
- Confirm no LLVM IR fallback was used and no expectations, allowlists, runner
  files, or timeout policy were weakened.
- Preserve exact proof output in `test_after.log`.

Completion check:

- `00005.c` and `00006.c` advance through the owned branch/control failure
  modes, prior proof cases remain green, and any later failure family is
  classified truthfully.

### Step 5: Review Residual Scope

Goal: decide whether idea 282 is complete or whether later failure families
should be split.

Actions:

- Compare the implementation and proof against
  `ideas/open/282_aarch64_loop_branch_control_runtime_hang.md`.
- Confirm no address/pointer storage, expectation, runner, timeout, or broad
  frontend/ABI work was folded into this plan.
- If complete, request plan-owner closure review.
- If proof exposes a distinct later issue, create or request a focused
  follow-up idea instead of expanding this plan.

Completion check:

- `todo.md` clearly says whether idea 282 is ready for closure review or names
  the exact blocker/follow-up owner layer.
