# Phase E Cross-Target Route-View Interface Reuse Runbook

Status: Active
Source Idea: ideas/open/189_phase_e_cross_target_route_view_interface_reuse.md

## Purpose

Reuse one AArch64-proven route-view interface at an x86 or riscv prepared
wrapper boundary without inventing a target-specific BIR adapter or moving
target policy into BIR.

## Goal

Select one bounded x86 `ConsumedPlans` or riscv prepared emission/wrapper
boundary, thread one already-proven semantic route view through that boundary,
and prove interface-level reuse while preserving target-local lowering policy.

## Core Rule

The selected target may consume an already-proven route view as semantic input
only. Do not move x86/riscv instruction selection, target formatting, frame
layout, register allocation, call ABI policy, emission records, or prepared
aggregate ownership into BIR.

## Read First

- `ideas/open/189_phase_e_cross_target_route_view_interface_reuse.md`
- `docs/bir_prealloc_fusion/phase_d_mir_consumer_switch_plan.md`
- The completed AArch64 Phase E route-view migrations for Route 3, Route 6,
  and Route 7
- The selected x86 `ConsumedPlans` or riscv prepared emission/wrapper boundary
- Existing route/debug, wrapper, and target emission tests for the selected
  boundary

## Current Scope

- One x86 `ConsumedPlans` or riscv prepared emission/wrapper boundary.
- One shared route view already proven by an AArch64 Phase E migration slice:
  Route 3 memory/access identity, Route 6 call-use source facts, or Route 7
  comparison/materialized-condition provenance.
- Interface-level compile, route/debug, wrapper, or target emission tests that
  prove the selected target consumes the shared semantic interface.

## Non-Goals

- Do not design a new x86-only or riscv-only BIR adapter before using a proven
  AArch64 route-view interface.
- Do not move target policy, ABI placement, frame layout, register allocation,
  instruction selection, target formatting, or emission records into BIR.
- Do not contract `PreparedFunctionLookups`, x86 consumed prepared wrappers, or
  riscv prepared-module emission as part of this runbook.
- Do not claim broad prepared aggregate contraction from one cross-target
  interface thread.
- Do not weaken route-debug, wrapper, or target emission coverage.

## Working Model

- AArch64 Route 3, Route 6, and Route 7 migrations are the prerequisite proof
  sources for this runbook.
- x86 and riscv can consume the same semantic route-view interface only at a
  wrapper boundary that already carries prepared context.
- Prepared target wrappers remain fallbacks and policy owners while the route
  view supplies semantic facts.
- Compile-only proof is acceptable only when the selected boundary has no
  behavioral, route/debug, or wrapper-observable contract to preserve.

## Execution Rules

- Keep the selected step behavior-preserving except for making the target
  wrapper aware of the shared route-view interface.
- Prefer existing route-view structs, facades, typed indexes, and validation
  helpers over new target-specific adapters.
- Keep target-local policy in x86 or riscv lowering code.
- Preserve prepared wrappers as fallback/oracle surfaces for any target policy
  still owned there.
- Fresh build or compile proof is required for every code-changing step; the
  supervisor owns broader validation selection.

## Ordered Steps

### Step 1: Select One Cross-Target Boundary

Goal: identify the bounded x86 or riscv wrapper boundary and the proven route
view it can consume.

Primary target: one x86 `ConsumedPlans` boundary or one riscv prepared
emission/wrapper boundary.

Actions:

- Inspect x86 `ConsumedPlans` and riscv prepared emission/wrapper surfaces.
- Choose one target boundary that can consume Route 3, Route 6, or Route 7
  semantic facts without needing a new target-specific BIR adapter.
- Confirm the selected route view was already proven by an AArch64 Phase E
  migration and identify the corresponding fallback/prepared target policy.
- Record in `todo.md` the selected target, boundary, route view, proof source,
  fallback path, and narrow proof command the supervisor should delegate next.
- Do not edit implementation during this mapping-only step unless the
  executor is explicitly delegated a code-changing packet.

Completion check:

- `todo.md` names the selected x86/riscv boundary, the reused Route 3/6/7
  interface, the target-local policy that stays out of BIR, and the initial
  proof command.

### Step 2: Thread the Proven Route View Through the Wrapper

Goal: make the selected x86 or riscv wrapper boundary consume the shared route
view where available.

Primary target: the selected target wrapper and the existing route-view
interface proven by AArch64.

Actions:

- Thread or expose the smallest existing route-view parameter or accessor
  needed by the selected wrapper.
- Preserve the target's existing prepared context and fallback behavior.
- Keep instruction selection, formatting, frame/register/ABI decisions, and
  emission-record construction target-owned.
- Avoid new broad BIR scans, target-specific route adapters, and
  `PreparedFunctionLookups` clones.

Completion check:

- The selected wrapper can consume the shared route view without changing
  target policy ownership.
- A fresh build or compile proof is recorded in `todo.md`.

### Step 3: Prove Interface Reuse and Fallback Coverage

Goal: prove the target wrapper reuses the shared interface without weakening
existing wrapper or target coverage.

Primary target: the selected target's route/debug, wrapper, compile, or target
emission tests.

Actions:

- Add or update tests that observe the selected target consuming the shared
  route-view interface.
- Preserve existing prepared-wrapper fallback behavior for absent or invalid
  route data.
- Include route/debug or wrapper-visible coverage when the touched boundary has
  observable route/debug behavior.
- Preserve target emission coverage when the selected boundary affects emitted
  target records.

Completion check:

- Tests prove interface reuse and fallback behavior for the selected boundary.
- The delegated narrow proof command is green and recorded in `todo.md`.

### Step 4: Final Route-Quality Check

Goal: make the cross-target reuse slice acceptance-ready for supervisor
review.

Primary target: the diff, `todo.md` proof notes, and the selected target
wrapper tests.

Actions:

- Check that no x86-only or riscv-only BIR adapter was invented.
- Check that target policy did not move into BIR.
- Check that no route-debug, wrapper, or target emission expectation was
  weakened.
- Check that the slice claims only one selected cross-target boundary, not
  broad prepared aggregate contraction.
- Ask the supervisor to run any broader proof needed for the touched target.

Completion check:

- `todo.md` records the final proof command and result.
- The diff demonstrates one shared route-view interface reuse at one target
  boundary.
