# RV64 Byval Aggregate Call ABI Plan

Status: Active
Source Idea: ideas/open/318_rv64_byval_aggregate_call_abi.md

## Purpose

Repair RV64 prepared call lowering for byval aggregate arguments so aggregate
payloads and addresses cross the call boundary through the target ABI instead
of truncating emitted assembly or faking callee-local state.

## Goal

Make the `src/00140.c` byval aggregate residual either run under qemu or split
only after concrete emitted-code evidence shows a different first bad
mechanism.

## Core Rule

Do not teach only `src/00140.c`, `%p.f`, one field name, or one stack-slot home
to pass. Progress must come from prepared call plans, aggregate layout, and
RV64 ABI-based byval aggregate transport.

## Read First

- ideas/open/318_rv64_byval_aggregate_call_abi.md
- build/rv64_c_testsuite_probe_latest/triage_314_step5/summary.md, if present
- Current RV64 prepared call argument, byval aggregate, aggregate layout,
  caller/callee, and floating-lane lowering under `src/backend/mir/`
- Existing backend tests for byval aggregate helpers, prepared call boundaries,
  RV64 calls, and qemu runtime behavior under `tests/backend/`

## Scope

- RV64 prepared call argument lowering for byval aggregate payloads.
- Caller-side aggregate-address and payload transport for byval arguments.
- Callee-side access to byval aggregate homes when supported by the call plan.
- Floating aggregate lane handling only where required for the narrow byval
  aggregate proof.
- Focused backend coverage for byval aggregate call semantics, including the
  shape exposed by `src/00140.c`.

## Non-Goals

- Nested aggregate-local subobject stores/loads already repaired by idea 314.
- Vararg ABI repair beyond what is inseparable from a focused byval aggregate
  proof.
- Broad RV64 floating-point ABI rewrites outside byval aggregate calls.
- Function pointer, indirect-call, or external runtime-call policy.
- Fake callee-local copies that bypass caller-side payload transport.
- Expectation rewrites, unsupported downgrades, or qemu skips claimed as ABI
  progress.

## Working Model

`src/00140.c` currently emits and links but qemu exits `132`, with assembly
truncated at `f1` byval aggregate handling. Prepared BIR exposes byval
aggregate-copy loads through `%p.f`, while the simple prepared call emitter
lacks caller-side aggregate-address and byval payload transport. Treat this as
a call-boundary ABI problem first, not as an aggregate-local store/load problem.

## Execution Rules

- Start from fresh BIR, prepared-BIR, assembly, link, and qemu evidence for
  `src/00140.c` before changing lowering.
- Add focused tests for byval aggregate call semantics, not for one candidate
  filename or `%p.f` spelling.
- Prove caller-side payload transport and callee-side aggregate field access;
  do not fake callee-local values.
- Keep aggregate-local, function-pointer, external-call, and select/control
  repairs stable.
- If vararg or broad floating ABI behavior becomes the first bad mechanism,
  classify it with emitted-code evidence instead of expanding this route
  without focused proof.
- Use the supervisor-selected proof command for each executor packet and write
  results into `test_after.log` unless delegated otherwise.

## Step 1: Normalize Byval Aggregate ABI Evidence

Goal: identify the first bad call-boundary mechanism in `src/00140.c`.

Primary target:

- `src/00140.c`

Actions:

- Reprobe `src/00140.c` for BIR dump, prepared-BIR dump, RV64 emit, clang link,
  and qemu outcome.
- Capture emitted RV64 assembly around the caller argument setup, call site,
  callee prologue, byval aggregate access, and truncation point.
- Inspect prepared call plans, byval aggregate records, aggregate layout, and
  `%p.f` access facts.
- Identify whether the first repair boundary is caller-side aggregate-address
  transport, payload copy, callee-side byval home access, floating aggregate
  lane handling, or a separate residual.
- Record focused tests to add.

Completion check:

- `todo.md` names the first repair boundary, cites prepared-BIR and emitted
  assembly evidence, and identifies focused byval aggregate call tests to add.

## Step 2: Add Focused Byval Aggregate Call Coverage

Goal: encode the byval aggregate call ABI behavior before changing RV64
lowering.

Primary targets:

- Backend tests for caller-side byval aggregate argument transport.
- Backend tests for callee-side aggregate field access through byval homes.
- Narrow floating aggregate lane coverage only if Step 1 shows it is required
  for the proof.

Actions:

- Add focused runtime or codegen-route coverage for passing an aggregate by
  value and reading fields in the callee.
- Add coverage that would fail if the caller never transports the aggregate
  payload or if the callee reads fake local values.
- Add focused floating-lane coverage only within the first bad mechanism
  selected in Step 1.
- Keep tests independent of `src/00140.c`, `%p.f`, field names, and fixed stack
  slots.

Completion check:

- Focused tests expose the current byval aggregate call gap and reject
  candidate-shaped or callee-fake shortcuts.

## Step 3: Repair Caller-Side Byval Payload Transport

Goal: move aggregate byval payloads through the RV64 call boundary according
to prepared call plans and target ABI rules.

Primary targets:

- RV64 prepared call argument lowering.
- Byval aggregate payload/address staging.
- Aggregate layout and call-boundary helper paths.

Actions:

- Inspect how prepared call plans represent the byval aggregate argument.
- Repair caller-side staging so the aggregate payload or address required by
  the ABI reaches the callee.
- Preserve ordinary direct call argument/result lowering.
- Re-run focused byval tests and the `src/00140.c` probe.

Completion check:

- Caller-side byval transport tests pass, and `src/00140.c` advances beyond
  the previous call-boundary truncation or exposes a classified next residual.

## Step 4: Repair Callee-Side Byval Home Access

Goal: make the callee read supported byval aggregate fields from the correct
incoming ABI location.

Primary targets:

- RV64 callee prologue/byval home setup.
- Aggregate field load lowering through byval homes.
- Floating aggregate lane handling only where selected by evidence.

Actions:

- Inspect how callee-side byval homes are represented after caller transport.
- Repair field access through the byval home without fabricating local copies.
- Add or refine focused tests for callee-side reads and any narrow floating
  lane handling required by the proof.
- Re-run focused tests and the `src/00140.c` probe.

Completion check:

- Callee-side byval aggregate tests pass, and `src/00140.c` either exits `0`
  under qemu or is classified as a separate residual with emitted-code
  evidence.

## Step 5: Reprobe And Close Classification

Goal: prove idea 318 acceptance criteria or preserve a separate residual for
future lifecycle work.

Primary targets:

- `src/00140.c`
- All focused byval aggregate call tests added during this runbook.

Actions:

- Reprobe `src/00140.c` for BIR dump, prepared-BIR dump, RV64 emit, clang link,
  and qemu outcome.
- Summarize which byval aggregate call ABI repairs landed.
- If any remaining failure is outside this idea, create or request a separate
  source idea with emitted-code evidence.
- Run the supervisor-selected backend guard or broader validation checkpoint.

Completion check:

- Focused backend coverage passes, `src/00140.c` either qemu-passes or is
  evidence-backed as a different residual, and no claimed progress depends on
  fake callee copies, expectation weakening, qemu skips, or candidate-shaped
  matching.
