# AArch64 C-Testsuite Backend Full Scan Runbook

Status: Active
Source Idea: ideas/open/230_aarch64_c_testsuite_backend_full_scan.md
Activated from: ideas/open/230_aarch64_c_testsuite_backend_full_scan.md

## Purpose

Establish an honest AArch64 backend scan over the vendored c-testsuite corpus,
classify the real failure surface, and preserve deeper repair work as
follow-up ideas or drafts instead of absorbing it into the scan.

## Goal

Register and run `tests/c/external/c-testsuite/src/*.c` through
`c4cll --codegen asm --target aarch64-unknown-linux-gnu`, then record how far
each case gets and group failures by owner layer.

## Core Rule

This runbook is discovery plus low-risk scan-path repair only. Do not turn
individual c-testsuite failures into broad AArch64 semantic repairs or
testcase-shaped shortcuts.

## Read First

- `ideas/open/230_aarch64_c_testsuite_backend_full_scan.md`
- `tests/c/external/CMakeLists.txt`
- `tests/c/external/c-testsuite/RunCase.cmake`
- Existing backend test registration and target-selection wiring near the
  c-testsuite CMake path.

## Current Targets

- Source corpus: `tests/c/external/c-testsuite/src/*.c`
- Expected outputs: existing `*.c.expected` sidecars
- Compiler route: `c4cll --codegen asm --target aarch64-unknown-linux-gnu`
- Assembly output: must be actual AArch64 backend assembly, not LLVM IR or host
  fallback output
- Assemble/link route: explicit AArch64 toolchain path, such as
  `clang --target=aarch64-unknown-linux-gnu`, when available
- Runtime route: direct AArch64 host execution or an explicitly configured
  supported runner/emulator

## Non-Goals

- Do not weaken c-testsuite cases, sidecars, tags, or expectations to create a
  green label.
- Do not add narrow named-case AArch64 lowering rules.
- Do not hide missing BIR, prepared-BIR, shared MIR, or ABI facts inside target
  codegen.
- Do not implement deeper call/frame, memory, scalar/float, i128, binary128,
  atomic, intrinsic, inline-asm, or variadic repairs inside this scan route.
- Do not count runtime-unavailable cases as backend passes.

## Working Model

The scan has three responsibilities:

1. make the AArch64 backend c-testsuite route explicit and runnable;
2. run the corpus once far enough to produce a reliable inventory;
3. split remaining deeper failure families into reviewable follow-up ideas or
   drafts with concrete failing-case evidence.

Easy fixes are allowed only when they repair registration, runner honesty,
toolchain wiring, path issues, or trivial low-risk backend bugs already covered
by prepared facts.

## Execution Rules

- Keep routine execution notes and proof in `todo.md`.
- Preserve failure inventory in a stable artifact or summary chosen by the
  executor and supervisor; do not leave root-level ad hoc `.log` files other
  than canonical regression logs.
- Separate compile, assemble/link, and runtime availability results.
- If the local environment cannot run AArch64 binaries, still inventory
  compile/assemble/link outcomes and mark runtime as unavailable, not passing.
- Create follow-up ideas or drafts for any nontrivial missing mechanism before
  claiming scan completion.

## Ordered Steps

### Step 1: Inspect Existing Registration

Goal: Identify how c-testsuite cases are currently registered and where backend
target selection is tied to host behavior.

Primary target:

- `tests/c/external/CMakeLists.txt`
- `tests/c/external/c-testsuite/RunCase.cmake`

Actions:

- Inspect current c-testsuite CMake registration, options, and generated test
  names.
- Inspect how `ENABLE_C_TESTSUITE_BACKEND_TESTS` selects a backend target.
- Identify where to thread an explicit AArch64 backend route without affecting
  unrelated host-backend tests.
- Decide the narrow proof command for the first registration change.

Completion check:

- The executor can name the exact registration and runner edits needed to
  invoke `--codegen asm --target aarch64-unknown-linux-gnu` over the vendored
  corpus.

### Step 2: Add Honest AArch64 Backend Scan Wiring

Goal: Register a clear AArch64 backend c-testsuite route over the vendored
corpus.

Primary target:

- c-testsuite CMake and runner wiring only.

Actions:

- Add or adjust registration so the AArch64 backend route is explicit in test
  names and command lines.
- Ensure the compiler emits AArch64 backend assembly and fails clearly on LLVM
  IR or host fallback output.
- Add explicit assemble/link handling for an AArch64 toolchain when available.
- Keep runtime execution conditional on a direct AArch64 host or intentionally
  configured supported runner/emulator.

Completion check:

- A narrow command lists or runs representative AArch64 backend c-testsuite
  cases, and the command line proves the AArch64 backend route rather than host
  fallback.

### Step 3: Run The First Full Scan

Goal: Execute the registered AArch64 backend c-testsuite route once and capture
the failure surface.

Actions:

- Run the full AArch64 backend c-testsuite route.
- Record counts for registered, compiled-to-AArch64-assembly, assembled/linked,
  run-and-matched, failed, and runtime-unavailable cases.
- Preserve the raw or summarized inventory in a stable artifact or summary
  acceptable to the supervisor.
- Do not convert failures into unsupported expectations during the scan.

Completion check:

- The scan result distinguishes frontend parse/sema, BIR/prepared-BIR,
  AArch64 lowering, fallback/non-AArch64 output, toolchain, runtime mismatch,
  and runtime-unavailable failures.

### Step 4: Repair Only Easy Scan-Path Failures

Goal: Fix low-risk blockers that prevent the scan route from being honest or
usable.

Actions:

- Repair missing test registration, path, runner, or build-system bugs that
  prevent the intended route from running.
- Repair only trivial backend defects already covered by prepared facts and
  localized enough to validate with the affected subset.
- Rerun the relevant subset after each accepted easy fix.

Completion check:

- Easy scan-path blockers have either landed with proof or are explicitly
  deferred with a reason in `todo.md`.

### Step 5: Split Deeper Failure Families

Goal: Preserve remaining nontrivial failures as reviewable follow-up ideas or
drafts.

Actions:

- Group remaining failures by likely owner layer: AArch64 lowering, shared MIR,
  BIR, prepared BIR, or toolchain/runtime environment.
- For each deeper mechanism gap, create a follow-up idea or draft with
  representative cases, observed commands/failures, owner layer, expected
  backend consumer, and proof subset to rerun.
- Do not activate deeper follow-up work during this runbook without explicit
  supervisor direction.

Completion check:

- Every remaining deeper failure family has concrete follow-up material, and no
  broad semantic repair is hidden inside the scan slice.

### Step 6: Finalize Scan Summary

Goal: Make the discovery result usable for subsequent AArch64 backend work.

Actions:

- Summarize final corpus counts, easy fixes, deferred blockers, and follow-up
  idea or draft paths.
- Confirm the scan route is runnable and the inventory distinguishes runtime
  unavailability from backend success.
- Ask the supervisor to decide whether the source idea is complete and ready
  for close.

Completion check:

- The active scan route, result inventory, and follow-up list are sufficient for
  a later agent to choose the next focused AArch64 backend repair idea without
  re-running discovery from scratch.
