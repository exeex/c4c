# RV64 Residual Pointer Array Select Flow Plan

Status: Active
Source Idea: ideas/open/316_rv64_residual_pointer_array_select_flow.md

## Purpose

Classify and repair the remaining RV64 pointer, local-array, and select
runtime residuals that were intentionally split out of idea 312 because they no
longer fit one local frame-slot address publication rule.

## Goal

Separate the residual candidates into coherent first-bad mechanisms, repair the
semantic pointer/array/select lowering rules that belong together, and split
any broad control-flow or unrelated residuals into durable follow-up ideas.

## Core Rule

Do not target one candidate filename, observed stack offset, or source shape.
Progress must come from a semantic RV64 pointer, local-array, or select value
lowering rule with nearby proof.

## Read First

- ideas/open/316_rv64_residual_pointer_array_select_flow.md
- Idea 312 closure notes, only if needed to distinguish completed local
  frame-slot address publication from these residuals
- Current RV64 prepared local-address, local-array, pointer conversion, select,
  and control-flow lowering under `src/backend/mir/`
- Existing backend tests for RV64 pointer local chains, local array pointer
  flow, select value materialization, and qemu runtime behavior under
  `tests/backend/`

## Scope

- Pointer-to-pointer local chains after the first dereference.
- Array parameter to local array pointer flow.
- Indexed local array select/update chains where local-array addressing is only
  one part of the failure.
- Pointer `select`, `inttoptr`, and `ptrtoint` conditional flow when it is the
  first bad runtime mechanism.
- Reclassification into narrower ideas if the four candidates split further.

## Non-Goals

- Core local frame-slot address publication already closed by idea 312.
- Aggregate-local subobject or byval repair tracked separately.
- Function-pointer address, return, or indirect-call repair tracked separately.
- Broad switch/control-flow repair unless it is required to prove a selected
  pointer or array value flow.
- Runtime-contract weakening, unsupported downgrades, or qemu skips claimed as
  progress.

## Working Model

Treat the four candidates as a triage cluster, not one assumed fix. The first
packet must identify the first bad mechanism for each case using prepared BIR,
emitted RV64 assembly, and qemu behavior. Only group implementation packets
when the evidence shows the same semantic lowering rule is responsible.

## Execution Rules

- Start with evidence for all four candidates before changing lowering.
- Add focused tests for the semantic mechanism being repaired, not for the
  c-testsuite filename.
- Keep completed idea 312 local-address behavior stable.
- Do not fold aggregate/byval or function-pointer work into this route.
- If `src/00143.c` becomes primarily switch/control-flow, split that into a
  follow-up idea instead of claiming pointer/array repair.
- If `src/00144.c` remains pointer-integer conversion or select-specific, keep
  proof focused on that value-flow mechanism.
- Use the supervisor-selected proof command for each executor packet and write
  results into `test_after.log` unless delegated otherwise.

## Step 1: Normalize Residual Candidate Evidence

Goal: classify all four candidates by first bad runtime mechanism.

Primary targets:

- `src/00005.c`
- `src/00077.c`
- `src/00143.c`
- `src/00144.c`

Actions:

- Reprobe each candidate for emit, link, and qemu outcome.
- Capture emitted RV64 assembly and any available prepared BIR dumps for the
  failing pointer, local-array, select, and control-flow regions.
- Identify whether each failure is pointer-to-pointer local flow, array
  parameter/local array pointer flow, indexed local array select/update flow,
  pointer select/integer conversion flow, broad switch/control flow, or another
  separate residual.
- Record the first coherent repair boundary and any candidates that should be
  split before implementation.

Completion check:

- `todo.md` assigns each candidate to a first-bad mechanism bucket, names the
  first repair boundary, and identifies focused tests to add for that boundary.

## Step 2: Add Focused Pointer/Array/Select Coverage

Goal: encode the selected semantic lowering gap before changing RV64 lowering.

Primary targets:

- Backend tests for the first coherent mechanism selected in Step 1.
- Neighboring same-feature cases that prove the route is not testcase-shaped.

Actions:

- Add focused runtime or codegen-route coverage for the selected
  pointer/array/select mechanism.
- Include nearby same-feature cases, such as a second pointer chain, local
  array pointer step, or pointer select/conversion shape, as appropriate.
- Ensure tests fail for stale values, missing pointer reloads, incorrect
  indexed addresses, incorrect select value materialization, or placeholder
  pointer-integer conversions.
- Avoid tests that only assert a single printed assembly spelling for one
  source file.

Completion check:

- Focused tests expose the current semantic gap and would reject filename,
  offset, or source-shape shortcuts.

## Step 3: Repair First Coherent Pointer/Array/Select Mechanism

Goal: implement the first semantic repair selected by Step 1 without expanding
into unrelated residuals.

Primary targets:

- RV64 prepared lowering for the selected pointer, local-array, or select
  value flow.
- Any shared prepared facts needed to carry the value through local memory,
  index computation, select materialization, or pointer conversion.

Actions:

- Inspect how the selected value flow reaches RV64 emission.
- Repair lowering so the runtime value uses the correct local memory,
  pointer-address, select-result, or pointer-integer conversion semantics.
- Preserve existing local frame-slot, aggregate, function-pointer, and direct
  call behavior.
- Re-run focused tests and the matching representative probes.

Completion check:

- Focused tests pass, and the representative candidates for the selected
  mechanism either run under qemu or expose a newly classified separate
  residual with emitted-code evidence.

## Step 4: Iterate Or Split Remaining Candidate Buckets

Goal: decide whether remaining candidates still belong in idea 316 or need
durable follow-up ideas.

Primary targets:

- Any of `src/00005.c`, `src/00077.c`, `src/00143.c`, or `src/00144.c` not
  resolved by Step 3.

Actions:

- Re-evaluate remaining failures against the source idea scope.
- If another pointer/array/select mechanism remains in scope, add focused
  coverage and repair it as the next coherent packet.
- If a remaining failure is broad switch/control-flow, ABI, aggregate, or
  function-pointer work, create or request a separate source idea rather than
  absorbing it.
- Record classifications and proof in `todo.md`.

Completion check:

- Remaining candidates are either repaired within the pointer/array/select
  scope or split into separate durable initiatives with concrete evidence.

## Step 5: Reprobe And Close Classification

Goal: prove idea 316 acceptance criteria or preserve separate residuals for
future lifecycle work.

Primary targets:

- `src/00005.c`
- `src/00077.c`
- `src/00143.c`
- `src/00144.c`
- All focused tests added during this runbook.

Actions:

- Reprobe all four candidates for emit, link, and qemu outcome.
- Summarize which semantic pointer/array/select repairs landed.
- Summarize any remaining candidates as separate mechanisms with concrete
  emitted-code evidence.
- Add follow-up ideas only for residuals outside this source idea's coherent
  scope.
- Run the supervisor-selected backend guard or broader validation checkpoint.

Completion check:

- Focused backend coverage passes, candidate outcomes are qemu passes or
  evidence-backed splits, and no claimed progress depends on filename,
  offset, source-shape matching, qemu skips, or weakened expectations.
