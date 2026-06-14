# Phase F3 x86 Route 3 LoadLocal Source-Memory Agreement Bridge

Status: Active
Source Idea: ideas/open/258_phase_f3_x86_route3_loadlocal_source_memory_agreement_bridge.md

## Purpose

Create the narrow x86 Route 3 agreement bridge needed before prepared
`memory_accesses` lookup answers can be treated as compatibility mirrors for a
selected `LoadLocal` result/source-memory identity.

## Goal

Add or prove one x86 consumer or MIR query facade that compares the selected
Route 3 `LoadLocal` memory record with the prepared `source_memory_access` row
and fails closed on non-agreement.

## Core Rule

Do not make prepared `memory_accesses` the semantic authority behind a renamed
Route 3/BIR accessor. Agreement must require both the Route 3 record and the
prepared source-memory row to describe the same selected identity.

## Read First

- `ideas/open/258_phase_f3_x86_route3_loadlocal_source_memory_agreement_bridge.md`
- Route 3 memory/source records and lookup helpers.
- x86 prepared memory consumers and MIR query surfaces.
- Focused backend tests covering prepared memory lookup/status, x86 route
  debug or handoff boundaries, and `LoadLocal` source-memory behavior.

## Current Targets

- Route 3 `LoadLocal` source-memory identity.
- `PreparedEdgePublication::source_memory_access` compatibility row.
- x86 agreement consumer or MIR query facade for the selected memory/source
  identity.
- Fail-closed rows for missing, incomplete, duplicate/conflict,
  prepared-only, route-only, unsupported, and mismatch cases.

## Non-Goals

- Do not implement generic memory parity beyond the selected `LoadLocal`
  source-memory fact.
- Do not change RISC-V behavior.
- Do not delete or privatize `PreparedFunctionLookups::memory_accesses`.
- Do not rewrite helper/oracle names, status strings, fallback names,
  prepared lookup/status contracts, x86 addressing, frame/global placement,
  operand spelling, register materialization, or target output formatting.
- Do not claim progress through expectation rewrites or named-case shortcuts.

## Working Model

- Route 3 owns the semantic memory/source fact for the selected `LoadLocal`.
- Prepared lookup/status remains the compatibility surface that existing
  callers and tests can observe.
- The bridge may answer agreement only when the x86-facing Route 3 record and
  prepared `source_memory_access` row are both present, complete, and matching.
- Target-owned x86 policy remains target-owned; the bridge must not absorb
  addressing, storage, register, or emitted-output decisions.

## Execution Rules

- Keep each implementation packet limited to the selected agreement path.
- Preserve public prepared lookup/status observability while adding the
  agreement gate.
- Add or update tests only to prove agreement and fail-closed behavior; do not
  weaken existing contracts.
- Use x86-enabled validation for x86-only tests when the default build does
  not register them.
- Escalate to plan-owner instead of broadening into generic memory lowering or
  target policy rewrites.

## Steps

### Step 1: Route 3 and x86 Memory Consumer Inventory

Goal: identify the exact Route 3 record fields, prepared source-memory fields,
and x86 reader or facade candidates that can participate in agreement.

Actions:
- Inspect Route 3 `LoadLocal` memory/source records and lookup helpers.
- Inspect x86 prepared memory consumers and MIR query wrappers.
- Record the candidate helper or facade boundary in `todo.md`.
- Identify existing tests that already cover prepared memory status and x86
  memory/source behavior.

Completion check:
- `todo.md` names the chosen reader/facade candidate, the Route 3 fields, the
  prepared row fields, and the compatibility surfaces that must remain stable.

### Step 2: Agreement Boundary and Fail-Closed Matrix

Goal: define the smallest agreement gate and rejection matrix before editing
  code.

Actions:
- Decide whether an existing x86 helper already forms the boundary or whether a
  narrow helper/facade is needed.
- Specify exact agreement predicates for same function, same block, selected
  `LoadLocal`, source-memory identity, and prepared row match.
- List rejection behavior for missing, incomplete, duplicate/conflict,
  prepared-only, route-only, unsupported, fallback, and mismatch rows.
- Record which rows require x86-enabled tests.

Completion check:
- `todo.md` records the accepted boundary and fail-closed matrix without
  implementation drift.

### Step 3: Implement or Prove the Narrow x86 Agreement Path

Goal: wire the selected x86 reader/facade through the agreement predicate or
prove that it is already correctly wired.

Actions:
- Add the minimal helper/facade code if no existing path satisfies the matrix.
- Keep prepared lookup/status and fallback names observable.
- Do not move x86 addressing, frame/global placement, operand spelling,
  register materialization, or target output policy into Route 3/BIR.
- Search for bypassing x86 selected `LoadLocal` memory/source consumers and
  either route them through the boundary or record why they are out of scope.

Completion check:
- Code, or explicit no-code proof, shows the selected x86 agreement path checks
  Route 3 and prepared source-memory agreement before treating prepared data as
  a semantic mirror.

### Step 4: Focused Agreement and Rejection Proof

Goal: prove the agreement path and fail-closed rows without weakening prepared
  compatibility contracts.

Actions:
- Add or update focused backend tests for the agreeing path and rejection rows.
- Include nearby same-feature cases, not only one named testcase.
- Run the supervisor-delegated proof command and write `test_after.log` when
  code or test behavior changes.
- Use an x86-enabled build/test configuration for x86-only CTest targets when
  required.

Completion check:
- Focused proof passes and covers agreement plus missing, incomplete,
  duplicate/conflict, prepared-only, route-only, unsupported, and mismatch
  rejection behavior expected by the source idea.

### Step 5: Compatibility Sweep and Closure Readiness

Goal: confirm the bridge is narrow, compatibility-preserving, and ready for
  lifecycle closure.

Actions:
- Re-check prepared lookup/status behavior, helper/oracle names, fallback
  names, x86 output stability, and RISC-V non-change.
- Confirm no unsupported expectation downgrades or output-baseline rewrites
  were used as proof.
- Record any remaining blocker in `todo.md`; otherwise request plan-owner
  closure after supervisor validation.

Completion check:
- `todo.md` records closure readiness or the exact blocker, and the proof logs
  cover the selected bridge at the confidence level requested by the
  supervisor.
