# Phase E4 x86 Route 6 Scalar i32 ConsumedPlans Compatibility Follow-up

Status: Active
Source Idea: ideas/open/238_phase_e4_x86_route6_scalar_i32_route_debug_consumedplans_compatibility_follow_up.md

## Purpose

Converge the single accepted x86 Route 6 scalar `i32` argument-source route-debug row so it consumes the proven Route 6 scalar source agreement fact while preserving `ConsumedPlans` compatibility and all x86 target-local call-wrapper policy.

## Goal

Route the selected x86 Route 6 scalar `i32` source-agreement path through the accepted Route 6 scalar source agreement fact without changing byte-stable x86 behavior, fallbacks, labels, wrappers, or baseline contracts.

## Core Rule

This is not broad x86 wrapper migration. Keep ABI placement, call-wrapper behavior, route-debug formatting, direct-call/helper-oracle behavior, wrapper output, and prepared call/debug fallback under existing x86 target-local policy.

## Read First

- `ideas/open/238_phase_e4_x86_route6_scalar_i32_route_debug_consumedplans_compatibility_follow_up.md`
- `ideas/closed/232_phase_e3_route6_x86_scalar_i32_argument_source_route_debug_follow_up.md`
- `docs/bir_prealloc_fusion/phase_e4_cross_target_wrapper_convergence_readiness.md`

## Current Targets And Scope

- One x86 Route 6 scalar `i32` argument-source route-debug row.
- The Route 6 scalar source agreement fact proven by closed idea `232`.
- Existing `ConsumedPlans`, prepared call plans, prepared call/debug fallback, direct-call helper behavior, helper-oracle behavior, route-debug formatting, wrapper output, and expected strings.
- Fail-closed behavior for absent, invalid, duplicate/conflict, mismatch, non-agreement, unsupported, public fallback, and policy-sensitive paths.

## Non-Goals

- Do not claim or implement riscv readiness.
- Do not perform broad x86 call-wrapper, ABI, call lowering, prepared call printer/debug, wrapper-family, or route-wide migration.
- Do not move ABI, frame layout, register allocation, instruction selection, emission, formatting, wrapper output, or call-wrapper policy into Route 6 or target-neutral BIR ownership.
- Do not delete, weaken, bypass, or hide `ConsumedPlans`, prepared call plans, prepared fallback, direct-call helpers, or helper-oracle compatibility.
- Do not use expected-string rewrites, helper renames, supported-path downgrades, unsupported relabeling, timeout masking, wrapper-output relabeling, or baseline refreshes as proof.
- Do not fold in aggregate `PreparedFunctionLookups`, `PreparedBirModule`, draft 155, E5 prepared aggregate retirement, or broad diagnostic/oracle replacement.

## Working Model

- Route 6 owns the scalar source agreement fact.
- x86 target-local code remains authoritative for placement, wrapper policy, formatting, direct calls, helper/carrier protocols, and output.
- `ConsumedPlans` and prepared call/debug fallback remain compatibility adapters for fallback and non-agreement behavior.
- The implementation should consume source agreement semantically, then prove nearby same-feature behavior rather than special-casing one fixture.

## Execution Rules

- Preserve behavior first; capability progress is only accepted when the source agreement fact is consumed without weakening existing contracts.
- Treat expectation rewrites, supported-path downgrades, and fixture-shaped matching as reject signals, not implementation tools.
- Keep changes narrowly scoped to the selected x86 Route 6 scalar `i32` path and its compatibility proof surface.
- For code-changing steps, run build proof and the supervisor-delegated narrow test subset; broader validation is required before acceptance if the diff touches shared call-wrapper, fallback, route-debug, helper-oracle, or baseline behavior.
- Keep `todo.md` as the packet-state scratchpad. Do not rewrite this runbook for routine executor progress.

## Ordered Steps

### Step 1. Locate the Selected Boundary

Goal: identify the exact selected x86 Route 6 scalar `i32` route-debug row, the source-agreement producer/consumer surfaces, and the compatibility adapters that must remain byte-stable.

Primary targets:
- Route 6 scalar source agreement path.
- x86 argument-source route-debug row generation.
- `ConsumedPlans`, prepared call/debug fallback, direct-call helper, helper-oracle, wrapper output, and expected-string surfaces.

Actions:
- Trace the accepted row from closed idea `232` and the Phase E4 readiness document into the current code and tests.
- Identify where the row currently re-derives or locally infers the source fact.
- Identify nearby same-feature positive and fail-closed cases for absent, invalid, duplicate/conflict, mismatch, non-agreement, fallback, direct-call/helper-oracle, wrapper, and expected-string behavior.
- Record the exact files, tests, and proof command candidates in `todo.md`.

Completion check:
- `todo.md` names the selected row, implementation surfaces, nearby proof cases, and the narrow proof command the supervisor can delegate for the implementation packet.

### Step 2. Consume Route 6 Scalar Source Agreement

Goal: change the selected x86 Route 6 scalar `i32` path to consume the proven Route 6 scalar source agreement fact instead of re-deriving the source locally.

Actions:
- Wire the selected path to the existing source-agreement fact using semantic plumbing rather than fixture-name matching.
- Preserve all x86 target-local decisions for ABI placement, call-wrapper policy, route-debug formatting, wrapper output, helper/carrier protocols, and direct-call behavior.
- Keep `ConsumedPlans`, prepared call plans, prepared call/debug fallback, direct-call helpers, and helper-oracle compatibility active for fallback and non-agreement paths.
- Avoid broad wrapper-family or route-wide migration.

Completion check:
- The selected positive row consumes Route 6 scalar source agreement and still produces byte-stable x86 route-debug, wrapper, helper-oracle, direct-call, fallback, and expected-string behavior under the delegated narrow proof.

### Step 3. Prove Fail-Closed And Compatibility Behavior

Goal: prove that source-agreement consumption did not weaken compatibility or hide old failure modes.

Actions:
- Exercise absent source facts.
- Exercise invalid source facts.
- Exercise duplicate/conflict source facts.
- Exercise mismatch and non-agreement behavior.
- Exercise public prepared fallback and `ConsumedPlans` compatibility.
- Exercise wrapper output stability.
- Exercise route-debug output stability.
- Exercise direct-call/helper-oracle behavior and status labels.
- Confirm expected-string stability and baseline-stability behavior without treating a baseline refresh as proof.

Completion check:
- `test_after.log` or the supervisor-designated proof artifact shows the required matrix, and `todo.md` records the exact pass/fail results plus any remaining gap.

### Step 4. Review For Overfit And Scope Drift

Goal: ensure the implementation is semantic, narrow, and still aligned with idea `238`.

Actions:
- Inspect the diff for named-case shortcuts, fixture-shaped matching, expectation rewrites, helper renames, supported-path downgrades, wrapper-output relabeling, or baseline-only proof.
- Confirm no riscv readiness, broad x86 wrapper migration, broad wrapper-family migration, or Route 6 ownership expansion was introduced.
- Confirm `ConsumedPlans`, prepared call plans, prepared fallback, direct-call helpers, and helper-oracle compatibility remain real compatibility adapters.
- Escalate to reviewer if route drift, broad migration, or testcase overfit is ambiguous.

Completion check:
- `todo.md` records an explicit route-quality result and any reviewer handoff path if review was required.

### Step 5. Prepare Acceptance Validation

Goal: leave the slice acceptance-ready for supervisor validation and lifecycle closure decision.

Actions:
- Ensure build proof is fresh for the final code slice.
- Ensure the delegated narrow proof covers the required proof matrix.
- Recommend broader or full validation if shared call-wrapper, fallback, route-debug, helper-oracle, or baseline surfaces were touched.
- Summarize completion against the source idea acceptance criteria in `todo.md`.

Completion check:
- The supervisor can decide commit readiness and whether to run broader validation without re-deriving the proof matrix or scope boundaries.
