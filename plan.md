# AArch64 Address-Exposed Local Pointer Runtime Nonzero Plan

Status: Active
Source Idea: ideas/open/281_aarch64_address_exposed_local_pointer_runtime_nonzero.md
Activated After: ideas/closed/278_aarch64_backend_local_operand_materialization_runtime_nonzero.md

## Purpose

Repair the next AArch64 backend runtime failure family exposed after scalar
local operand materialization for `00003.c` was fixed.

## Goal

Make address-exposed local and pointer semantics lower coherently enough for
`tests/c/external/c-testsuite/src/00004.c` and `00005.c` to advance through the
AArch64 backend runtime route without expectation changes.

## Core Rule

Fix the semantic backend/codegen rule for address-exposed locals or pointer
memory operands. Do not use filename-specific matching, c-testsuite shortcuts,
LLVM IR fallback, expectation weakening, or broad ABI/register-allocation
rewrites as progress.

## Read First

- `ideas/open/281_aarch64_address_exposed_local_pointer_runtime_nonzero.md`
- `todo.md`
- `ideas/closed/278_aarch64_backend_local_operand_materialization_runtime_nonzero.md`
- `ideas/open/282_aarch64_loop_branch_control_runtime_hang.md`
- `test_after.log`, if still present, for the latest focused route evidence
- AArch64 backend lowering, prepared memory operand, address formation,
  load/store, storage-plan, and assembly emission code touched by
  address-exposed locals or pointers

## Current Targets

- AArch64 backend route for `00004.c` and `00005.c`.
- Address-exposed local storage, pointer value materialization, load/store
  address formation, and prepared memory operand consumption.
- Regression proof for idea 278 focused cases: `00001.c`, `00002.c`, and
  `00003.c`.
- Focused backend proof that covers the repaired memory/pointer operand rule
  outside the exact c-testsuite filenames where practical.

## Non-Goals

- Do not rework loop or branch control lowering for `00006.c`; that belongs to
  `ideas/open/282_aarch64_loop_branch_control_runtime_hang.md`.
- Do not change c-testsuite `.expected` sidecars, allowlists, unsupported
  classifications, CTest contracts, runner files, or route diagnostics.
- Do not add filename-specific handling for `00004.c` or `00005.c`.
- Do not route proof through LLVM IR fallback while claiming AArch64 backend
  progress.
- Do not perform broad ABI, runtime-runner, or register-allocation rewrites
  unless tracing proves they are the direct owner of this address-exposed local
  family.

## Working Model

Idea 278 repaired ordinary scalar local operand materialization. The broad
AArch64 backend scan then advanced through `00001.c`, `00002.c`, and
`00003.c`, but `00004.c` and `00005.c` fail as `[RUNTIME_NONZERO] exit=1`.

Those failures are later than the immediate scalar local store/load owner. The
likely owner is address-exposed local storage, pointer value materialization,
load/store address formation, or prepared memory operand consumption.

## Execution Rules

- Keep each packet bounded to investigation, one semantic repair, or one proof
  update.
- Update `todo.md` with the current packet result and exact proof command.
- Preserve `test_after.log` as the canonical executor proof log for routine
  packets unless the supervisor delegates another artifact.
- Prefer AST-backed symbol queries via `c4c-clang-tools` for large C++ backend
  files.
- Treat a change as route drift if its main effect is making only `00004.c` or
  `00005.c` pass without repairing address-exposed local or pointer semantics.
- Keep `00006.c` branch/loop control evidence out of this plan except as a
  follow-up pointer to idea 282.

## Steps

### Step 1: Localize Address-Exposed Local Ownership

Goal: identify the backend path that lowers address-taken locals and pointer
reads/writes for the failing AArch64 runtime cases.

Actions:

- Reproduce or inspect the focused `00004.c` and `00005.c` AArch64 backend
  route evidence.
- Compare the generated assembly against the source-level pointer/addressed
  local behavior.
- Trace where address-exposed local storage, pointer values, and memory
  operands are represented before AArch64 lowering.
- Identify whether the owner is storage publication, pointer materialization,
  address formation, load/store selection, or prepared memory operand
  consumption.
- Record the smallest semantic repair point and a focused proof command in
  `todo.md`.

Completion check:

- `todo.md` names the owner function or backend stage, explains why `00004.c`
  and `00005.c` currently return nonzero, and gives the exact proof subset for
  the repair packet.

### Step 2: Repair Address-Exposed Local Or Pointer Lowering

Goal: lower the supported address-exposed local or pointer form coherently for
AArch64 machine output.

Actions:

- Implement the smallest semantic backend/codegen change at the owner point
  identified in Step 1.
- Preserve scalar local operand materialization from idea 278.
- Preserve result-register behavior from idea 277.
- Keep unsupported forms fail-closed or truthfully classified at the owner
  layer.
- Build the affected backend target before runtime proof.

Completion check:

- The project builds for the affected backend target, and generated assembly
  for the motivating cases uses coherent address-exposed local or pointer
  storage semantics rather than unrelated ABI registers or filename-shaped
  shortcuts.

### Step 3: Prove Runtime And Regression Coverage

Goal: prove the repair through the AArch64 backend route and reject testcase
overfit.

Actions:

- Run the focused AArch64 backend route for `00004.c` and `00005.c`.
- Rerun `00001.c`, `00002.c`, and `00003.c` to prove idea 278 remains green.
- Run focused backend memory/pointer operand coverage that exercises the same
  semantic rule outside the exact c-testsuite filenames where practical.
- Confirm no LLVM IR fallback was used and no expectations were weakened.
- Preserve exact proof output in `test_after.log`.

Completion check:

- `00004.c` and `00005.c` advance through the AArch64 backend runtime route
  without expectation weakening, and idea 278 proof cases remain green.

### Step 4: Review Residual Scope

Goal: decide whether idea 281 is complete or whether later failure families
should remain split.

Actions:

- Compare the implemented behavior and proof against
  `ideas/open/281_aarch64_address_exposed_local_pointer_runtime_nonzero.md`.
- Confirm no loop/branch control work for `00006.c` was folded into this plan.
- If complete, request plan-owner closure review.
- If proof exposes a distinct backend/codegen/BIR issue, create or request a
  focused follow-up idea instead of expanding this plan.

Completion check:

- `todo.md` clearly says whether idea 281 is ready for closure review or names
  the exact blocker/follow-up owner layer.
