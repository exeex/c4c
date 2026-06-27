# RV64 gcc_torture Runtime Abort And Segfault Mismatches Runbook

Status: Active
Source Idea: ideas/open/402_rv64_gcc_torture_runtime_abort_and_segfault_mismatches.md
Activated after: ideas/closed/410_lir_to_bir_packed_bitfield_scalar_binop_semantics.md

## Purpose

Classify and repair RV64 gcc_torture cases that compile, link, and run through
qemu but diverge from clang with aborts or segmentation faults.

## Goal

Turn the current runtime-mismatch bucket into concrete backend correctness
families, then repair or split the first supportable family without weakening
qemu comparison.

## Core Rule

Runtime mismatch work must preserve observable program semantics. Do not make
cases return zero by weakening comparison, skipping qemu, filtering allowlists,
or masking bad BIR/prepared facts in RV64 code.

## Read First

- `ideas/open/402_rv64_gcc_torture_runtime_abort_and_segfault_mismatches.md`
- `ideas/open/354_rv64_gcc_torture_prepared_module_shape_classification.md`
- `tests/c/external/gcc_torture/src/20000113-1.c`
- `tests/c/external/gcc_torture/src/20070212-2.c`
- Current RV64 gcc_torture backend scan artifacts under `build/agent_state/`
  and `build/rv64_gcc_c_torture_backend/`

## Current Targets

- Abort representative: `tests/c/external/gcc_torture/src/20000113-1.c`
- Segfault representative: `tests/c/external/gcc_torture/src/20070212-2.c`
- Nearby runtime abort/segfault cases selected by the supervisor from current
  RV64 gcc_torture backend artifacts.

## Non-Goals

- Do not treat compile-fail, prepared-handoff, object-route diagnostic, or
  timeout cases as part of this runtime bucket.
- Do not weaken qemu comparison contracts or expected exits.
- Do not use compile-only proof for cases that reach qemu.
- Do not mask incorrect BIR/prepared facts in MIR/RV64 lowering; route
  producer defects separately.
- Do not use filename-specific branches, expectation rewrites, unsupported
  downgrades, or allowlist filtering.

## Working Model

The reopened 354 classification found 34 runtime mismatches:

- 23 `clang_exit=0 c4c_exit=Subprocess aborted`
- 11 `clang_exit=0 c4c_exit=Segmentation fault`

These cases have advanced past object emission and link, so Step 1 must triage
actual qemu behavior, emitted objects, and prepared facts before any repair.
Abort and segfault cases may be distinct families; split them if the evidence
does not justify a shared owner.

## Execution Rules

- Start with classification proof before implementation.
- Keep each implementation packet tied to one concrete runtime root-cause
  family.
- Compare clang and c4c qemu behavior for any repaired runnable case.
- Inspect emitted assembly/object dumps, prepared facts, and qemu traces where
  useful before editing backend lowering.
- If the mismatch is caused by incorrect BIR/prepared facts, stop and route
  that producer boundary instead of compensating in RV64 lowering.
- Use the supervisor-selected proof command and record exact results in
  `todo.md`.
- Treat return-code-only wins, diagnostic-only churn, expectation rewrites,
  allowlist filtering, and named-case green proof as route failures.

## Step 1: Classify Current Runtime Mismatch Families

Goal: identify concrete root-cause families for the current abort and segfault
representatives.

Actions:

- Reproduce or inspect the supervisor-selected RV64 gcc_torture backend probe
  for `20000113-1.c`, `20070212-2.c`, and nearby same-bucket runtime cases.
- Record clang exit, c4c exit, qemu stderr/stdout, object/link status, and the
  first observable divergence for each representative.
- Inspect prepared dumps and emitted assembly/object dumps around the failing
  path.
- Decide whether the first repair packet is an RV64 lowering bug, ABI/call
  convention bug, memory/addressing bug, prepared producer fact bug, or a
  narrower split.
- Route unrelated or producer-owned residuals to lifecycle review instead of
  patching runtime symptoms.

Completion check:

- `todo.md` records the concrete runtime family for the first executor packet,
  the representative set, and the exact supervisor-delegated proof command.
- Abort and segfault cases are either grouped with evidence or split with
  precise ownership.

## Step 2: Repair The First Runtime Root Cause

Goal: repair the first classified runtime mismatch family while preserving
observable semantics.

Actions:

- Update the correct backend or producer layer for the selected root cause.
- Preserve ABI, memory, control-flow, data, and runtime semantics.
- Add or update focused coverage where the repo has a matching test surface.
- Keep fail-closed diagnostics for unsupported adjacent shapes.

Completion check:

- The selected representative no longer has the same runtime abort/segfault
  behavior under qemu.
- The repaired representative matches clang behavior for the proof surface or
  advances to a precisely routed new residual.
- Existing backend tests for adjacent lowering remain green.

## Step 3: Prove Representatives And Residual Ownership

Goal: prove the runtime repair advanced 402 without hiding correctness
failures.

Actions:

- Run the supervisor-selected RV64 gcc_torture backend proof for the repaired
  representative and same-family additions.
- Inspect qemu comparison for each case that compiles and links.
- Run the supervisor-selected backend CTest subset for the implementation
  slice.
- Classify any remaining failure as the same runtime family, a distinct
  runtime family, a target-emission family, or a producer-fact gap.

Completion check:

- `todo.md` records exact proof commands, results, and residual ownership.
- Runtime mismatch count does not increase in the refreshed proof subset.
- No expectation rewrites, qemu weakening, allowlist filtering, masked
  producer defects, or filename-specific fixes are used as acceptance
  evidence.
- The supervisor has enough evidence to continue with another 402 packet,
  request route review, or ask the plan owner for close/deactivation handling.
