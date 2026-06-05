# AArch64 Remaining Baseline Failure Recovery

Status: Active
Source Idea: ideas/open/110_aarch64_remaining_baseline_failure_recovery.md

## Purpose

Recover the current remaining AArch64 baseline failures before returning to
BIR/prealloc cleanup.  Treat the preserved baseline log as evidence, but prove
against current `HEAD` before changing code.

## Goal

Make the current remaining AArch64 narrow failure set pass, then refresh broader
AArch64 baseline confidence without weakening expectations.

## Core Rule

Fix semantic lowering, data-flow ownership, or ABI handling defects.  Do not add
named-testcase shortcuts, unsupported downgrades, or expectation rewrites as
progress.

## Read First

- `ideas/open/110_aarch64_remaining_baseline_failure_recovery.md`
- `test_baseline.log`
- `log/baseline_93a0ca1a852132daa7ed40d61055b20c95aedd88.log`
- `log/baseline_9f348520108810254973de09888a576c1a81d41d.log`
- C sources for the current target tests
- AArch64 codegen, BIR, and prealloc surfaces touched only as needed by the
  characterized failure family

## Current Targets

- `backend_aarch64_instruction_dispatch`
- `c_testsuite_aarch64_backend_src_00172_c`
- `c_testsuite_aarch64_backend_src_00180_c`
- `c_testsuite_aarch64_backend_src_00216_c`
- `c_testsuite_aarch64_backend_src_00220_c`
- `c_testsuite_aarch64_backend_src_00204_c` only as stale-baseline confirmation

## Non-Goals

- Do not work idea 109 BIR/prealloc residue cleanup inside this plan.
- Do not start x86 or RISC-V backend work.
- Do not broadly reorganize `src/backend/mir/aarch64/codegen` unless a current
  failure requires a narrow movement.
- Do not update expected outputs to hide backend defects.

## Working Model

The preserved baseline records six failures, but current targeted evidence
shows `00204` passing after the stdarg HFA repair.  The first execution packet
must refresh the current failure facts and classify the remaining failures by
shared capability before repair packets are chosen.

Early clues:

- `backend_aarch64_instruction_dispatch` concerns selected `f64` global readback
  feeding a call ABI move.
- `00172`, `00180`, `00216`, and `00220` likely involve pointer/string
  mutation, aggregate initialization or copy shape, local aggregate ordering,
  and one segmentation fault.

## Execution Rules

- Keep each repair packet tied to one failure family or one shared lowering
  cause.
- Prefer AST-backed symbol queries for large C++ surfaces before raw-file
  spelunking.
- Update `todo.md` after each packet with the current step, proof command, and
  remaining failures.
- Use the exact narrow proving command until the current target set is clean:

```sh
ctest --test-dir build -j1 --output-on-failure -R '^(backend_aarch64_instruction_dispatch|c_testsuite_aarch64_backend_src_00(172|180|216|220|204)_c)$'
```

- Escalate to broader AArch64 validation only after the narrow set is clean or
  after a high-blast-radius shared lowering change.

## Steps

### Step 1: Refresh Current Failure Facts

Goal: establish the current failing set at `HEAD` and separate stale baseline
entries from live failures.

Actions:

- Run the narrow target command.
- Capture each failing test's current symptom in `todo.md`.
- Confirm whether `00204` passes or still fails in the same rerun.
- Inspect the matching C sources and the internal dispatch test assertion.
- Group failures by likely shared backend capability.

Completion check:

- `todo.md` records the current pass/fail set and a proposed next repair step
  ordered by likely shared cause, not by testcase number alone.

### Step 2: Repair Instruction Dispatch Floating-Point Call Feed

Goal: fix the selected `f64` global readback path so the internal instruction
dispatch expectation reflects a valid call ABI feed.

Actions:

- Inspect the failing internal test and emitted assembly.
- Trace the selected floating-point value through BIR/prealloc/AArch64 codegen.
- Fix the semantic lowering or register/feed ownership issue without tailoring
  to the assertion string.
- Re-run the narrow target command.

Completion check:

- `backend_aarch64_instruction_dispatch` passes and no current C target regresses
  in the narrow set.

### Step 3: Repair Pointer and String Mutation Failures

Goal: fix the live failure family covering `00172` and `00180`, if current
classification confirms they share pointer or string mutation lowering.

Actions:

- Inspect the C sources and wrong-output deltas.
- Identify whether the defect lives in address materialization, pointer
  carrier/value-home behavior, memory-base selection, or store/load sequencing.
- Implement a semantic fix at the lowest shared layer that benefits the full
  failure family.
- Re-run the narrow target command.

Completion check:

- `00172` and `00180` pass, or `todo.md` records why they are separate families
  and which one remains.

### Step 4: Repair Aggregate Ordering and Crash Failures

Goal: fix the live failure family covering `00216` and `00220`, if current
classification confirms aggregate initialization/copy or local layout handling
is the shared cause.

Actions:

- Inspect the C sources and runtime symptoms.
- Trace aggregate layout, local initialization, copy, and address-use lowering
  across BIR/prealloc/AArch64.
- Fix the shared semantic issue without adding named-case conditionals.
- Re-run the narrow target command.

Completion check:

- `00216` and `00220` pass, or `todo.md` records a justified split and the
  remaining live family.

### Step 5: Refresh Baseline Confidence

Goal: prove the recovered target set and update stale baseline understanding.

Actions:

- Re-run the narrow target command.
- If clean, run the repo-standard broader AArch64 baseline command.
- Compare the new result against `test_baseline.log` and the preserved log under
  `log/`.
- Record whether `00204` was stale-only in the preserved baseline.

Completion check:

- The narrow target set is clean.
- Broader AArch64 validation has no unexplained new failures.
- `todo.md` contains closure-ready proof notes for idea 110.
