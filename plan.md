# Prepared Wide Rematerialized Immediate Admission Runbook

Status: Active
Source Idea: ideas/open/404_prepared_wide_rematerialized_immediate_admission.md
Activated after: ideas/closed/411_rv64_object_route_rhs_bitfield_boolean_select_abort.md

## Purpose

Repair the producer/prepared handoff for wide rematerialized immediate values
that reach RV64 object emission without an explicit destination home or
consumer-ready fact.

## Goal

Classify and repair the first supportable wide rematerialized immediate
producer family so `src/int-compare.c` no longer fails first because prepared
facts leave a wide immediate value homeless.

## Core Rule

Keep this in the BIR/prepared producer contract. RV64 object emission should
consume explicit prepared facts, not reconstruct arbitrary BIR constant
arithmetic from instruction shape.

## Read First

- `ideas/open/404_prepared_wide_rematerialized_immediate_admission.md`
- `ideas/closed/401_rv64_object_route_scalar_and_floating_edge_lowering.md`
- `tests/c/external/gcc_torture/src/int-compare.c`
- Prepared pure-instruction admission and rematerialized-immediate handling
  in the backend/prepared pipeline.
- Existing immediate, compare, and RV64 object-emission backend tests.

## Current Targets

- Primary representative: `tests/c/external/gcc_torture/src/int-compare.c`
- Producer family: wide rematerialized immediates such as `INT_MIN` built from
  `bir.sub i32 0, 2147483647` followed by subtracting `1`.
- Boundary: prepared values that are semantically complete but have no
  destination home or consumer-ready materialization fact.

## Non-Goals

- Do not reopen scalar compare/trunc or floating-cast lowering from 401.
- Do not implement unrelated scalar `ashr`, bitfield-global lowering, or
  generic RV64 instruction fragments.
- Do not teach RV64 object emission to infer missing BIR constant-expression
  semantics.
- Do not special-case `int-compare.c`, `INT_MIN`, or exact immediate spelling.
- Do not use expectation rewrites, unsupported downgrades, allowlist filtering,
  or diagnostic-only churn as progress.

## Working Model

The current failure is a prepared producer/admission boundary: a wide immediate
producer is known and semantically complete, but prepared facts do not provide
an admissible home or reusable materialization contract for RV64 object
emission. Step 1 should prove the exact first bad fact before implementation.

## Execution Rules

- Start with a fresh classification of `int-compare.c` before editing code.
- Preserve signedness, width, and materialization semantics for wide constants.
- Prefer publishing explicit producer facts or rejecting incomplete producers
  at the right layer over compensating in RV64 object emission.
- Add focused backend coverage for the repaired producer family.
- Run the supervisor-selected build and backend proof command, and record exact
  results in `todo.md`.
- Route any unrelated later residual to a separate owner instead of expanding
  404.

## Step 1: Classify Wide Immediate Producer Facts

Goal: identify the exact prepared producer gap for the first failing
`int-compare.c` wide rematerialized immediate.

Actions:

- Reproduce or inspect the current `int-compare.c` backend route failure.
- Dump semantic and prepared BIR around the wide immediate producers.
- Record whether the value should publish a destination home, a reusable
  constant-materialization fact, or a stricter producer-side rejection.
- Identify nearby representative cases for the same producer family if they are
  already visible from existing proof artifacts.

Completion check:

- `todo.md` records the first bad fact, the exact representative set, and the
  supervisor-delegated proof command for the first implementation packet.
- Any non-404 residual is routed with concrete evidence.

## Step 2: Repair Prepared Wide Immediate Admission

Goal: make the producer/prepared contract explicit for the classified wide
rematerialized immediate family.

Actions:

- Update the correct producer/prepared admission path so wide immediate values
  have consumer-ready facts or fail closed with a precise producer diagnostic.
- Preserve signedness, bit width, rematerialization, and constant-materializing
  behavior.
- Add or update focused backend coverage for the repaired wide immediate
  family and adjacent immediate forms.

Completion check:

- `src/int-compare.c` no longer fails first on a no-home wide rematerialized
  immediate producer.
- Focused backend tests cover the new prepared contract and adjacent existing
  immediate behavior remains green.

## Step 3: Prove Representative And Residual Ownership

Goal: prove 404 advanced without hiding unrelated RV64 or producer failures.

Actions:

- Rerun the supervisor-selected representative proof for `int-compare.c` and
  any nearby same-family cases identified in Step 1.
- Run the supervisor-selected backend CTest subset.
- Inspect the first remaining failure, if any, and classify it as same-family,
  a distinct producer gap, a target object-emission gap, or a semantic route
  outside 404.

Completion check:

- `todo.md` records exact proof commands, statuses, and residual ownership.
- The old no-home wide rematerialized immediate failure is absent or precisely
  explained as not fixed.
- The supervisor has enough evidence to continue, request review, split a
  residual, or ask the plan owner for close/deactivation handling.
