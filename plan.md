# Phase E5 Route 3 Memory Source Identity Adapter Runbook

Status: Active
Source Idea: ideas/open/240_phase_e5_route3_memory_source_identity_adapter_follow_up.md

## Purpose

Implement one narrow Route 3 memory/source identity adapter while preserving
prepared lookup delivery, target policy, diagnostics, fallback, wrapper
behavior, expected strings, and baselines.

## Goal

Route exactly one selected Route 3 `memory_accesses` identity read through an
agreement-gated adapter that consumes route/BIR semantic identity only when it
matches the existing prepared lookup answer.

## Core Rule

Treat Route 3 as semantic identity only. Do not move address formation,
materialization, relocation, final operands, value homes, wrappers,
diagnostics, fallback, or target emission policy into Route 3 ownership.

## Read First

- `ideas/open/240_phase_e5_route3_memory_source_identity_adapter_follow_up.md`
- `docs/bir_prealloc_fusion/phase_e5_prepared_bir_module_demotion_or_retirement_gate.md`
- `docs/bir_prealloc_fusion/prepared_function_lookups_ownership_readiness_map.md`
- `docs/bir_prealloc_fusion/residual_route_view_consumer_migration_map.md`
- `ideas/closed/202_route3_memory_source_identity_adapter.md`
- `ideas/closed/190_full_suite_baseline_99_percent_regression_attribution.md`

## Current Scope

- One Route 3 memory/source identity read inside `memory_accesses`.
- One agreement-gated adapter for that read.
- Prepared fallback for absent, invalid, duplicate/conflict, ambiguous,
  mismatch, unsupported, prepared-only, policy-sensitive, and non-agreement
  cases where they apply to the selected reader.
- Unchanged prepared lookup delivery for diagnostics, helper-oracle tests,
  wrappers, compatibility consumers, and debug/printer paths.
- Nearby same-feature memory/source proof that the adapter is semantic rather
  than fixture-shaped.

## Non-Goals

- Do not delete, privatize, or replace all of `memory_accesses`.
- Do not delete, privatize, rename, or hide `PreparedFunctionLookups` or
  `PreparedBirModule`.
- Do not open draft 155 or claim aggregate retirement readiness.
- Do not change address formation, address materialization, relocation, final
  memory operands, value homes, stack/frame/register policy, wrapper output,
  or target emission policy ownership.
- Do not rewrite expectations, helper names, supported-path status, wrapper
  output, or baselines as proof.
- Do not downgrade supported tests to unsupported or weaken prepared fallback,
  diagnostic/oracle, debug/printer, wrapper, or helper contracts.

## Working Model

- Existing Route 3 records may answer selected memory/source identity
  questions such as memory access id, source value/global/local identity,
  block/instruction compatibility, and same-block provenance.
- Prepared memory/access lookup surfaces remain authoritative for target
  policy, public compatibility, oracles, diagnostics, and fallback.
- A valid adapter first obtains the current prepared lookup answer, then
  accepts a route/BIR semantic answer only when the route fact agrees with
  that prepared answer.
- Any missing, ambiguous, mismatched, invalid, unsupported, policy-sensitive,
  or non-agreement state must fall back to the existing prepared path.

## Execution Rules

- Name the selected reader before implementation. If the reader consumes
  target policy instead of semantic identity, stop and select a narrower read
  or return to the supervisor.
- Keep the implementation to one reader and one adapter boundary.
- Preserve public prepared lookup APIs and prepared debug/printer/helper
  behavior unless a separate source idea explicitly changes them.
- Add capability proof around the adapter and nearby same-feature cases; do
  not rely on one named fixture.
- Treat expectation rewrites, baseline refreshes, helper renames, unsupported
  downgrades, facade-only moves, and named-case shortcuts as reject signals.
- For code-changing steps, run a fresh build or compile proof plus the
  supervisor-selected narrow test subset. Escalate to broader validation if
  the diff touches shared prepared lookup, diagnostic/oracle, wrapper, or
  target-policy surfaces.

## Steps

### Step 1: Select The Route 3 Memory/Source Reader

Goal: identify exactly one semantic Route 3 memory/source identity read that is
eligible for an agreement-gated adapter.

Primary targets:

- Route 3 `memory_accesses` readers in AArch64 memory/source paths.
- Existing helper surfaces such as `PreparedMemoryAccessLookups`,
  `find_prepared_memory_access(...)`,
  `find_prepared_global_load_access(...)`,
  `find_prepared_same_block_global_load_access(...)`, and load-local source
  helpers.
- Candidate source files including `src/backend/mir/aarch64/codegen/memory.cpp`,
  `src/backend/mir/aarch64/codegen/globals.cpp`,
  `src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp`,
  and adjacent Route 3 helper code.

Actions:

- Inspect candidate readers and choose one that asks only for memory/source
  identity, not address formation or target policy.
- Record the chosen reader, route fact, prepared lookup answer, fallback path,
  and nearby same-feature cases in `todo.md`.
- Identify the exact tests or oracle rows that prove positive agreement,
  missing route fact, mismatch, ambiguity/conflict if applicable, and prepared
  fallback.

Completion check:

- `todo.md` names one selected reader and explains why it is semantic identity
  only, with no target-policy ownership transfer.

### Step 2: Implement The Agreement-Gated Adapter

Goal: route the selected reader through one adapter that accepts Route 3
semantic identity only after agreement with the prepared lookup result.

Primary targets:

- The selected reader from Step 1.
- The Route 3 semantic identity helper or record used by that reader.
- The existing prepared lookup fallback path.

Actions:

- Add the smallest local adapter or helper needed to compare the Route 3 fact
  with the prepared answer.
- Preserve the prepared fallback path for absent, invalid, ambiguous,
  mismatch, unsupported, prepared-only, policy-sensitive, and non-agreement
  cases relevant to the selected reader.
- Keep address formation, relocation choice, materialization, final operand
  construction, value homes, wrappers, diagnostics, and emission policy on the
  existing prepared/target path.
- Avoid public API hiding, aggregate reshaping, and rename-only changes.

Completion check:

- The selected positive path can consume the route/BIR semantic identity after
  prepared agreement, while every non-agreement path keeps existing prepared
  behavior.

### Step 3: Prove Fallback And Public Compatibility

Goal: prove the adapter preserves prepared fallback, diagnostics, public
lookup delivery, and same-feature behavior.

Primary targets:

- Existing Route 3 memory/source tests and prepared lookup helper/oracle tests.
- Prepared printer/debug, route-debug, wrapper, and target-output surfaces
  touched by the selected reader.

Actions:

- Add or update focused tests for the selected positive agreement case.
- Cover absent, invalid, duplicate/conflict, ambiguous, mismatch, unsupported,
  prepared-only, policy-sensitive, and non-agreement fallback where applicable.
- Include nearby same-feature memory/source cases so the proof is not tied to
  one named fixture.
- Verify helper-oracle names and status labels, route-debug output, prepared
  printer/debug output, wrapper output, expected strings, supported-path
  contracts, and baselines remain byte-stable unless separately approved.

Completion check:

- The narrow proof demonstrates semantic adapter behavior and preserved public
  prepared compatibility without expectation downgrades or baseline refreshes.

### Step 4: Validate Target-Policy No-Change Surfaces

Goal: prove the adapter did not move target-policy or output ownership into
Route 3.

Primary targets:

- Adjacent address, frame, register, materialization, relocation, value-home,
  final memory operand, wrapper, and target-output surfaces.
- The supervisor-selected build and test subset.

Actions:

- Run fresh build or compile proof after implementation.
- Run the delegated narrow CTest subset for the selected reader and nearby
  memory/source cases.
- Inspect the diff for expectation rewrites, baseline refreshes, unsupported
  downgrades, helper renames, facade-only moves, aggregate reshuffles, or
  named-case shortcuts.
- Escalate to broader validation if shared prepared lookup, wrapper,
  diagnostic/oracle, or target-policy surfaces were modified.

Completion check:

- `todo.md` records passing proof and confirms target-policy, wrapper,
  expected-string, and baseline behavior remained unchanged.

### Step 5: Prepare Closure Evidence

Goal: make the completed Route 3 adapter slice reviewable for lifecycle
closure without broadening the source idea.

Primary targets:

- `todo.md`
- The selected code and test diff.
- Canonical proof logs chosen by the supervisor.

Actions:

- Summarize the selected reader, adapter boundary, fallback cases, nearby
  same-feature proof, and no-change target-policy evidence.
- Record exact validation commands and log paths.
- State explicitly that no whole `memory_accesses`,
  `PreparedFunctionLookups`, `PreparedBirModule`, or draft 155 retirement
  readiness is claimed.

Completion check:

- `todo.md` contains closure-ready evidence for the plan owner to decide
  whether source idea 240 is complete.
