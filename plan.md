# RV64 Select Chain Short Circuit Runtime Lowering Plan

Status: Active
Source Idea: ideas/open/317_rv64_select_chain_short_circuit_runtime_lowering.md

## Purpose

Repair the RV64 runtime residual where select-chain or short-circuit
control/value flow emits and links but computes the wrong result.

## Goal

Make the `src/00046.c` select-chain/short-circuit residual either run under
qemu with the expected result or split only after concrete emitted-code evidence
shows a different first bad mechanism.

## Core Rule

Do not special-case `src/00046.c`, fixed aggregate offsets, or one exact source
expression. Progress must come from RV64 prepared select-chain and
short-circuit control/value lowering.

## Read First

- ideas/open/317_rv64_select_chain_short_circuit_runtime_lowering.md
- build/rv64_c_testsuite_probe_latest/triage_314_step5/summary.md, if present
- Current RV64 prepared control-flow, select-chain materialization, branch, and
  value-publication lowering under `src/backend/mir/`
- Existing backend tests for prepared select-chain, short-circuit, aggregate
  local, and RV64 qemu runtime behavior under `tests/backend/`

## Scope

- RV64 prepared select-chain materialization and short-circuit runtime lowering.
- Control/value joins that feed select-chain results in emitted RV64 code.
- Focused backend coverage for select-chain and short-circuit semantics,
  including the shape exposed by `src/00046.c`.
- Emitted-code evidence that distinguishes control/select failure from local
  aggregate storage failure.

## Non-Goals

- Reopening aggregate-local subobject stores/loads repaired by idea 314.
- Generic local frame-slot address publication from idea 312.
- Broad boolean/control-flow rewrites unrelated to select-chain or
  short-circuit materialization.
- Byval aggregate call ABI, vararg ABI, or floating aggregate lane repair.
- Expectation rewrites, unsupported downgrades, or skipped qemu checks claimed
  as runtime progress.

## Working Model

Idea 314 already moved `src/00046.c` past nested aggregate store/reload
emission. Treat the remaining qemu exit as a select-chain or short-circuit
runtime problem until emitted-code and prepared-BIR evidence prove a different
first bad mechanism.

## Execution Rules

- Start from fresh evidence for `src/00046.c` before changing lowering.
- Add focused tests that describe select-chain or short-circuit semantics, not
  the c-testsuite filename.
- Preserve aggregate-local focused tests from idea 314.
- Consume or improve prepared select-chain/control facts rather than adding
  target-local rescans that duplicate authority.
- If the candidate reaches an unrelated control, ABI, aggregate, or storage
  residual, classify it with emitted-code evidence instead of absorbing it.
- Use the supervisor-selected proof command for each executor packet and write
  results into `test_after.log` unless delegated otherwise.

## Step 1: Normalize Select-Chain Residual Evidence

Goal: identify the first bad runtime mechanism for `src/00046.c` after the
aggregate-local repair.

Primary target:

- `src/00046.c`

Actions:

- Reprobe `src/00046.c` for emit, link, and qemu outcome.
- Capture emitted RV64 assembly and prepared BIR for the select-chain,
  short-circuit, and control/value join regions.
- Confirm aggregate stores/reloads remain present and are not the first bad
  mechanism.
- Identify whether the failure is select-chain materialization,
  short-circuit branch/control emission, value publication, or a separate
  residual.
- Record focused backend tests to add.

Completion check:

- `todo.md` names the first repair boundary, cites emitted-code evidence, and
  identifies focused tests for the selected select-chain or short-circuit
  mechanism.

## Step 2: Add Focused Select/Short-Circuit Coverage

Goal: encode the selected runtime semantics before changing RV64 lowering.

Primary targets:

- Backend tests for RV64 select-chain and short-circuit runtime behavior.
- Existing aggregate-local focused tests as non-regression neighbors.

Actions:

- Add focused runtime or codegen-route coverage for the selected
  select-chain/short-circuit shape.
- Include at least one nearby same-feature case that is not `src/00046.c`.
- Ensure tests would fail for stale select values, missing control joins,
  target-local expression matching, or skipped qemu execution.
- Keep coverage independent of fixed aggregate offsets and exact source
  spelling.

Completion check:

- Focused tests expose the current select-chain/short-circuit gap and would
  reject filename or expression-shaped shortcuts.

## Step 3: Repair Select-Chain Or Short-Circuit Lowering

Goal: implement the selected semantic repair without reopening aggregate-local
storage.

Primary targets:

- RV64 prepared select-chain materialization.
- RV64 short-circuit branch/control emission.
- Prepared control/value join facts consumed by RV64 codegen.

Actions:

- Inspect how the selected prepared facts reach RV64 emission.
- Repair lowering so the runtime select/control value is materialized or
  branched correctly.
- Preserve existing aggregate-local stores/reloads and prior RV64 pointer,
  function-pointer, and external-call behavior.
- Re-run focused tests, aggregate-local neighbors, and the `src/00046.c` probe.

Completion check:

- Focused select/short-circuit tests pass, aggregate-local neighbors remain
  green, and `src/00046.c` either exits `0` under qemu or is classified as a
  newly exposed separate residual with concrete emitted-code evidence.

## Step 4: Reprobe And Close Classification

Goal: prove idea 317 acceptance criteria or preserve a separate residual for
future lifecycle work.

Primary targets:

- `src/00046.c`
- All focused select-chain/short-circuit tests added during this runbook.
- Aggregate-local focused neighbors from idea 314.

Actions:

- Reprobe `src/00046.c` for emit, link, and qemu outcome.
- Summarize which select-chain or short-circuit repair landed.
- If any remaining failure is outside this idea, create or request a separate
  source idea with emitted-code evidence.
- Run the supervisor-selected backend guard or broader validation checkpoint.

Completion check:

- Focused backend coverage passes, `src/00046.c` either qemu-passes or is
  evidence-backed as a different residual, and no claimed progress depends on
  expectation weakening, qemu skips, fixed offsets, or source-shape matching.
