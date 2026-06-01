# Shared Aggregate Transport Plan Probe Runbook

Status: Active
Source Idea: ideas/open/75_shared_aggregate_transport_plan_probe.md

## Purpose

Probe whether aggregate copy chunk/lane planning and transport decomposition
need shared prepared aggregate-transport authority before any AArch64 migration.

## Goal

Name the duplicated aggregate transport decisions, decide whether a shared
prepared plan/query is required, and only then allow a bounded implementation
route.

## Core Rule

Do not migrate or rewrite AArch64 aggregate copy lowering until the probe names
the missing authority and the candidate prepared transport fact shape.

## Read First

- `ideas/open/75_shared_aggregate_transport_plan_probe.md`
- `src/backend/mir/aarch64/codegen/instruction.cpp`
- `src/backend/mir/aarch64/codegen/memory.cpp`
- Existing shared prealloc/value-home/addressing aggregate surfaces that may
  already own chunk, lane, home, scratch, or transport-kind facts.

## Current Scope

- Trace aggregate copy decomposition across AArch64 instruction and memory
  owners.
- Identify repeated decisions for chunk boundaries, lane bindings,
  source/destination homes, scratch needs, and transport kind.
- Draft a prepared aggregate-transport plan/query shape if the duplication is
  real.
- Migrate AArch64 consumers only after the shared contract is explicit.

## Non-Goals

- Do not treat ordinary load/store opcode choice or address spelling as shared
  aggregate transport authority.
- Do not combine this route with general memory address cleanup.
- Do not rewrite aggregate test expectations to avoid missing authority.
- Do not add named aggregate-copy testcase shortcuts.

## Working Model

- Stack-backed, register-backed, and mixed or lane-sensitive aggregate
  transports are the required comparison families when present.
- Target-local instruction selection and final AArch64 record spelling remain
  in AArch64.
- Shared authority, if needed, should describe transport facts before target
  lowering consumes them.

## Execution Rules

- Keep source-idea edits unnecessary unless the durable intent changes.
- Record probe findings in `todo.md` before code changes.
- Prefer small packets: inventory, contract draft, then implementation.
- Every code-changing step needs a fresh build plus focused aggregate tests.
- Use canonical regression logs before accepting code slices.

## Step 1: Inventory Aggregate Transport Decisions

Goal: prove whether the suspected duplication is real and where it lives.

Primary targets:
- `src/backend/mir/aarch64/codegen/instruction.cpp`
- `src/backend/mir/aarch64/codegen/memory.cpp`
- relevant shared aggregate/value-home/addressing lookup surfaces

Actions:
- Trace stack-backed aggregate copy decomposition.
- Trace register-backed aggregate copy decomposition.
- Trace mixed or lane-sensitive aggregate copy decomposition if present.
- For each path, list chunk boundaries, lane bindings, source homes,
  destination homes, scratch needs, and transport kind decisions.
- Classify each decision as already prepared/shared, AArch64-local by design,
  or missing shared authority.

Completion check:
- `todo.md` contains a concise inventory covering the required transport
  families or explains why a family is absent.
- The next step has a concrete target: existing shared authority to consume, a
  missing prepared aggregate-transport shape to draft, or a no-code conclusion.

## Step 2: Draft Prepared Aggregate-Transport Contract

Goal: define the shared fact/query shape only if Step 1 proves missing shared
authority.

Actions:
- Name the proposed prepared aggregate-transport fields for chunks, lanes,
  source/destination homes, scratch needs, and transport kind.
- Identify the shared owner that should publish or query the plan.
- Define what remains target-local in AArch64 lowering.
- Record reject conditions for any migration that would still re-derive the
  same plan locally.

Completion check:
- `todo.md` or a plan checkpoint contains an implementable contract draft with
  owner, query shape, consumer responsibilities, and proof expectations.
- If no shared contract is needed, the plan is ready for lifecycle review
  without implementation.

## Step 3: Implement Shared Transport Authority If Required

Goal: add the prepared aggregate-transport plan/query and migrate only the
  duplicated AArch64 decisions that Step 2 assigned to shared authority.

Actions:
- Add or extend shared prepared aggregate/value-home/addressing surfaces for
  the agreed contract.
- Migrate AArch64 aggregate copy lowering to consume prepared transport facts.
- Keep target-local opcode choice, record construction, and address spelling in
  AArch64.
- Avoid broad memory cleanup outside the named transport facts.

Completion check:
- Focused aggregate copy tests prove stack-backed, register-backed, and mixed
  or lane-sensitive transports where applicable.
- Build proof is fresh.
- Regression guard accepts matching before/after logs for the chosen scope.

## Step 4: Acceptance Review And Closure Decision

Goal: decide whether idea 75 is complete or whether a follow-up idea is needed.

Actions:
- Compare the implementation and probe notes against the source idea.
- Reject testcase-shaped shortcuts, expectation downgrades, helper-only
  renames, or broad memory cleanup.
- Run or consume a broader regression guard appropriate to the touched surface.
- If a separate initiative remains, create it under `ideas/open/` instead of
  expanding this plan silently.

Completion check:
- A reviewer can see whether shared aggregate transport authority exists, was
  unnecessary, or was deferred into a separate source idea.
- The active lifecycle state is ready for close, deactivation, or plan rewrite.
