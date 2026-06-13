# Phase E5 Route 6 Scalar Call-Use Source Identity Row Runbook

Status: Active
Source Idea: ideas/open/242_phase_e5_route6_scalar_call_use_source_identity_row_follow_up.md

## Purpose

Implement one narrow Route 6 scalar call-use source identity row or reader
while preserving prepared call plans, `ConsumedPlans`, public fallback,
target-local call policy, diagnostics, wrappers, expected strings, and
baselines.

## Goal

Route exactly one selected Route 6 scalar call argument or result source
identity row through an agreement-gated adapter that consumes the semantic
source fact only when it matches the existing prepared call-plan answer.

## Core Rule

Treat Route 6 as semantic source identity only. Do not move ABI placement,
frame layout, register allocation, call-wrapper policy, helper/carrier
protocols, result lanes, outgoing stack layout, formatting, instruction
selection, emission policy, or wrapper output into Route 6 or target-neutral
BIR ownership.

## Read First

- `ideas/open/242_phase_e5_route6_scalar_call_use_source_identity_row_follow_up.md`
- `docs/bir_prealloc_fusion/phase_e5_prepared_bir_module_demotion_or_retirement_gate.md`
- `docs/bir_prealloc_fusion/prepared_function_lookups_ownership_readiness_map.md`
- `docs/bir_prealloc_fusion/residual_route_view_consumer_migration_map.md`
- `docs/bir_prealloc_fusion/phase_d2_retained_surface_consumer_switch_analysis.md`
- `ideas/closed/205_route6_call_use_source_adapter.md`
- `ideas/closed/238_phase_e4_x86_route6_scalar_i32_route_debug_consumedplans_compatibility_follow_up.md`
- `ideas/closed/239_phase_e5_prepared_bir_module_demotion_or_retirement_gate.md`

## Current Scope

- One selected Route 6 scalar call-use source identity row or reader for a call
  argument or result role.
- One agreement-gated adapter for that selected row or reader.
- Prepared fallback for absent, invalid, duplicate/conflict, ambiguous,
  mismatch, unsupported, prepared-only, policy-sensitive, and non-agreement
  cases where they apply to the selected row.
- Unchanged prepared call plans, `ConsumedPlans`, public prepared call/debug
  fallback, direct-call/helper-oracle families, diagnostics, wrappers,
  compatibility consumers, and debug/printer paths.
- Nearby same-feature scalar call-use proof that the adapter is semantic
  rather than fixture-shaped.

## Non-Goals

- Do not implement broad x86 call-wrapper migration, route-wide x86 readiness,
  riscv readiness, or cross-target wrapper convergence.
- Do not delete, privatize, rename, hide, or replace whole `call_plans`,
  `PreparedFunctionLookups`, or `PreparedBirModule`.
- Do not open draft 155 or claim aggregate retirement readiness.
- Do not move ABI placement, frame layout, register allocation, call-wrapper
  policy, helper/carrier protocol, result lanes, outgoing stack layout,
  formatting, instruction selection, emission policy, or wrapper output into
  Route 6 or target-neutral BIR ownership.
- Do not rewrite expectations, helper names, supported-path status, wrapper
  output, or baselines as proof.
- Do not downgrade supported tests to unsupported or weaken prepared call
  plans, `ConsumedPlans`, public fallback, diagnostic/oracle, debug/printer,
  wrapper, or helper contracts.

## Working Model

- Route 6 records may answer selected scalar call-use source identity questions
  for a call argument or result role.
- Prepared call-plan and lookup surfaces remain authoritative for ABI
  placement, wrapper kind, clobbers, outgoing stack sizing, byval lanes,
  variadic FPR counts, helper/carrier protocols, value homes, move bundles,
  publication routing, aggregate transport, final call records, diagnostics,
  public fallback, and emitted output.
- A valid adapter first obtains the current prepared call-plan answer, then
  accepts a Route 6 semantic source answer only when the route fact agrees with
  that prepared answer.
- Any missing, ambiguous, mismatched, invalid, unsupported, policy-sensitive,
  prepared-only, or non-agreement state must fall back to the existing prepared
  path.
- Closed idea 238 is prerequisite evidence only for the x86 Route 6 scalar
  `i32` route-debug / `ConsumedPlans` compatibility boundary. Do not treat it
  as broad wrapper, riscv, or aggregate retirement evidence.

## Execution Rules

- Name the selected row or reader before implementation. If it consumes ABI,
  wrapper, helper/carrier, storage, formatting, or emission policy instead of
  semantic source identity, stop and select a narrower row or return to the
  supervisor.
- Keep the implementation to one row or reader and one adapter boundary.
- Preserve public prepared call-plan APIs and prepared debug/printer/helper
  behavior unless a separate source idea explicitly changes them.
- Add capability proof around the adapter and nearby same-feature scalar
  call-use cases; do not rely on one named fixture.
- Treat expectation rewrites, baseline refreshes, helper renames, unsupported
  downgrades, facade-only moves, broad wrapper claims, and named-case shortcuts
  as reject signals.
- For code-changing steps, run a fresh build or compile proof plus the
  supervisor-selected narrow test subset. Escalate to broader validation if the
  diff touches shared prepared call-plan, `ConsumedPlans`, diagnostic/oracle,
  wrapper, or target-policy surfaces.

## Steps

### Step 1: Select The Route 6 Scalar Call-Use Source Reader

Goal: identify exactly one semantic Route 6 scalar call-use source identity
row or reader that is eligible for an agreement-gated adapter.

Primary targets:

- Route 6 scalar call argument or result source identity rows.
- Existing prepared call-plan readers and lookup helpers for call argument or
  result source facts.
- `ConsumedPlans`, prepared call/debug fallback, direct-call/helper-oracle,
  wrapper, and route-debug compatibility surfaces.
- Candidate source files and tests discovered from those helper names.

Actions:

- Inspect candidate readers and choose one that asks only for scalar call-use
  source identity, not ABI placement, call-wrapper policy, helper/carrier
  protocol, result lanes, outgoing stack layout, formatting, instruction
  selection, or emission policy.
- Record the chosen reader, Route 6 fact, prepared call-plan answer, fallback
  path, and nearby same-feature cases in `todo.md`.
- Identify the exact tests or oracle rows that prove positive agreement,
  missing route fact, mismatch, ambiguity/conflict if applicable, policy
  sensitivity, public fallback, `ConsumedPlans` compatibility, and unchanged
  wrapper/output behavior.

Completion check:

- `todo.md` names one selected row or reader and explains why it is semantic
  source identity only, with no target/prepared policy ownership transfer.

### Step 2: Implement The Agreement-Gated Adapter

Goal: route the selected row or reader through one adapter that accepts Route 6
semantic source identity only after agreement with the prepared call-plan
result.

Primary targets:

- The selected row or reader from Step 1.
- The Route 6 semantic source identity helper or record used by that reader.
- The existing prepared call-plan fallback path.

Actions:

- Add the smallest local adapter or helper needed to compare the Route 6 fact
  with the prepared call-plan answer.
- Preserve the prepared fallback path for absent, invalid, ambiguous,
  duplicate/conflict, mismatch, unsupported, prepared-only, policy-sensitive,
  and non-agreement cases relevant to the selected reader.
- Keep prepared call plans, `ConsumedPlans`, ABI placement, wrapper kind,
  clobbers, outgoing stack sizing, byval lanes, variadic FPR counts,
  helper/carrier protocols, value homes, move bundles, publication routing,
  aggregate transport, final call records, diagnostics, formatting, and emitted
  output on the existing prepared/target path.
- Avoid public API hiding, aggregate reshaping, broad wrapper migration, and
  rename-only changes.

Completion check:

- The selected positive path can consume the Route 6 semantic source identity
  after prepared agreement, while every non-agreement or policy-sensitive path
  keeps existing prepared behavior.

### Step 3: Prove Fallback And Public Compatibility

Goal: prove the adapter preserves prepared call-plan fallback, diagnostics,
public lookup delivery, `ConsumedPlans` compatibility, and same-feature
behavior.

Primary targets:

- Existing Route 6 scalar call-use source tests and prepared lookup
  helper/oracle tests.
- Prepared call printer/debug, x86 route-debug, wrapper, direct-call/helper,
  and target-output surfaces touched by the selected reader.

Actions:

- Add or update focused tests for the selected positive agreement case.
- Cover absent, invalid, duplicate/conflict, ambiguous, mismatch, unsupported,
  prepared-only, policy-sensitive, public fallback, and non-agreement fallback
  where applicable.
- Include nearby same-feature scalar call-use cases so the proof is not tied
  to one named fixture.
- Verify helper-oracle names and status labels, route-debug output, prepared
  printer/debug output, wrapper output, expected strings, supported-path
  contracts, and baselines remain byte-stable unless separately approved.
- Verify idea 238 remains only narrow x86 Route 6 scalar `i32` route-debug /
  `ConsumedPlans` evidence and is not generalized into broad wrapper or
  aggregate readiness.

Completion check:

- The narrow proof demonstrates semantic adapter behavior and preserved public
  prepared compatibility without expectation downgrades, wrapper relabeling, or
  baseline refreshes.

### Step 4: Validate Target-Policy No-Change Surfaces

Goal: prove the adapter did not move call policy or output ownership into Route
6.

Primary targets:

- Adjacent ABI placement, frame, register, stack, helper/carrier, result-lane,
  outgoing-stack, call-wrapper, formatting, instruction-selection, emission,
  wrapper, and target-output surfaces.
- The supervisor-selected build and test subset.

Actions:

- Run fresh build or compile proof after implementation.
- Run the delegated narrow CTest subset for the selected row and nearby scalar
  call-use cases.
- Inspect the diff for expectation rewrites, baseline refreshes, unsupported
  downgrades, helper renames, facade-only moves, aggregate reshuffles, broad
  wrapper claims, or named-case shortcuts.
- Escalate to broader validation if shared prepared call-plan,
  `ConsumedPlans`, wrapper, diagnostic/oracle, or target-policy surfaces were
  modified.

Completion check:

- `todo.md` records passing proof and confirms target-policy, wrapper,
  expected-string, and baseline behavior remained unchanged.

### Step 5: Prepare Closure Evidence

Goal: make the completed Route 6 scalar call-use source adapter slice
reviewable for lifecycle closure without broadening the source idea.

Primary targets:

- `todo.md`
- The selected code and test diff.
- Canonical proof logs chosen by the supervisor.

Actions:

- Summarize the selected row or reader, adapter boundary, fallback cases,
  nearby same-feature proof, and no-change target-policy evidence.
- Record exact validation commands and log paths.
- State explicitly that no broad x86 call-wrapper migration, route-wide x86
  readiness, riscv readiness, cross-target wrapper convergence, whole
  `call_plans`, `PreparedFunctionLookups`, `PreparedBirModule`, or draft 155
  retirement readiness is claimed.
- State explicitly that closed idea 238 remains narrow prerequisite evidence
  only for the x86 Route 6 scalar `i32` route-debug / `ConsumedPlans`
  compatibility boundary.

Completion check:

- `todo.md` contains closure-ready evidence for the plan owner to decide
  whether source idea 242 is complete.
