# Route 3 Memory/Source Semantic Reader Runbook

Status: Active
Source Idea: ideas/open/207_route3_memory_source_semantic_reader.md

## Purpose

Transcribe the Route 3 memory/source follow-up into an implementation runbook for exactly one AArch64 semantic reader.

## Goal

Move one named AArch64 memory/source semantic reader to prefer selected Route 3 source identity only when it agrees with prepared lookup evidence, while preserving prepared fallback, target-addressing policy, wrappers, diagnostics, and expected strings.

## Core Rule

Route 3 may supply semantic memory/source identity for the selected reader, but prepared memory/addressing and target code remain authoritative for address formation, materialization, final operands, and every fallback-sensitive path.

## Read First

- `ideas/open/207_route3_memory_source_semantic_reader.md`
- `docs/bir_prealloc_fusion/phase_d2_retained_surface_consumer_switch_analysis.md`
- `docs/bir_prealloc_fusion/phase_c2_selected_adapter_cache_contraction_readiness.md`
- `docs/bir_prealloc_fusion/residual_route_view_consumer_migration_map.md`
- `docs/bir_prealloc_fusion/prepared_function_lookups_ownership_readiness_map.md`
- `ideas/closed/190_full_suite_baseline_99_percent_regression_attribution.md`
- `ideas/closed/202_route3_memory_source_identity_adapter.md`

## Current Targets

- One named AArch64 memory/source semantic reader.
- Route 3 source identity lookup/index surfaces already present in the codebase.
- The prepared lookup or helper path currently used by the selected reader.
- Tests or expected-output checks that cover route/prepared agreement, absent Route 3 facts, invalid references, mismatches, target-addressing fallback, x86 wrapper stability, and byte-stable expected strings.

## Non-Goals

- Do not migrate address formation, frame/global/TLS relocation, offsets, address spaces, base-plus-offset legality, materialization, final operands, or target policy.
- Do not remove prepared fallback, delete public prepared APIs, retire `PreparedFunctionLookups`, or migrate whole memory/access lookup families.
- Do not change wrappers, prepared printer/debug output, helper-oracle strings, supported-path contracts, baselines, or expected strings to claim progress.
- Do not broaden this plan to diagnostic/oracle rows from `ideas/open/208_route3_memory_source_oracle_printer_row.md`.

## Working Model

- The selected reader must continue to compute or obtain the prepared answer.
- A Route 3 fact can be consumed only as an agreement-checked semantic identity source.
- Missing, unusable, or disagreeing Route 3 facts must fail closed to the exact prepared behavior.
- Target-addressing and address-materialization outputs are proof surfaces for non-regression, not migration targets.

## Execution Rules

- Name the exact reader before implementation and keep the rest of the memory/source family out of scope.
- Prefer local adapter/helper patterns that already enforce route/prepared agreement.
- Keep code changes near the selected reader and any existing Route 3 semantic lookup helper.
- Add or adjust tests only to prove the semantic rule and fallback matrix; do not rewrite expected strings unless a separate semantic proof requires it and the supervisor approves.
- For code-changing steps, run a fresh build or compile proof plus the delegated narrow test subset. Escalate broader validation if public headers, shared helpers, wrappers, or expected-output surfaces are touched.

## Ordered Steps

### Step 1: Select The Reader And Baseline Surface

Goal: Identify the one AArch64 memory/source semantic reader that can consume Route 3 identity without taking over target-addressing policy.

Primary targets:
- AArch64 memory, globals, calls, or ALU code paths that currently ask prepared memory/source helpers for semantic source identity.
- Existing Route 3 memory/source index or adapter APIs.
- Existing tests covering the selected reader and nearby fallback behavior.

Actions:
- Inspect candidate readers and choose exactly one named reader.
- Record why the selected reader asks for semantic source identity rather than address formation or operand policy.
- Identify the prepared fallback answer and the Route 3 source identity evidence that must agree.
- Identify narrow proof commands and expected no-change output surfaces for the executor packet.

Completion check:
- `todo.md` names one selected reader, the prepared fallback surface, the Route 3 evidence source, and the proof subset before implementation begins.

### Step 2: Add Agreement-Gated Route 3 Read

Goal: Make the selected reader prefer Route 3 only when Route 3 and prepared lookup agree.

Primary targets:
- The selected reader from Step 1.
- The smallest existing helper or new local helper needed to compare Route 3 identity with the prepared result.

Actions:
- Keep the prepared lookup path available and authoritative.
- Read Route 3 identity for the selected reader's source only after the prepared answer is available or otherwise comparable.
- Accept the Route 3 identity only for a valid same-source agreement.
- Fall back to prepared behavior for absent facts, invalid references, and route/prepared mismatches.

Completion check:
- The selected reader has route/prepared agreement behavior, and all non-agreement paths return the existing prepared result.

### Step 3: Prove Fallback And Policy Boundaries

Goal: Prove the change is semantic-reader work only, not target-addressing or output migration.

Primary targets:
- Positive agreement test for the selected reader.
- Absent, invalid, mismatch, and policy-sensitive fallback tests.
- Prepared printer/debug, x86 wrapper, and expected-output checks that should remain byte-stable.

Actions:
- Add or extend focused tests for positive, absent, invalid, and mismatch behavior.
- Include at least one fallback-sensitive case showing address formation or address-materialization output remains prepared-owned.
- Run the delegated narrow proof command and any required compile/build command.
- Avoid expected-string churn; if an expected string changes unexpectedly, stop and report the attribution instead of refreshing it.

Completion check:
- Narrow proof is green, expected strings and wrapper behavior remain unchanged, and `todo.md` records the exact command output summary.

### Step 4: Finalize Slice State

Goal: Leave the lifecycle state ready for supervisor review and commit.

Primary targets:
- `todo.md`
- `test_after.log` when delegated by the supervisor or used as the executor proof artifact.

Actions:
- Update `todo.md` with the selected reader, implementation summary, proof commands, and any residual risks.
- Confirm no out-of-scope implementation, wrapper, diagnostic/oracle, baseline, or expected-string changes were made.
- If proof exposes a separate diagnostic/oracle or policy initiative, record it as a follow-up note instead of expanding this plan.

Completion check:
- `todo.md` reports the completed packet and proof; the diff remains scoped to one Route 3 semantic reader and its focused tests.

## Validation

- Minimum acceptance for implementation packets: fresh build or compile proof plus the supervisor-delegated narrow Route 3 memory/source test subset.
- Required behavioral proof: positive route/prepared agreement, absent Route 3 fallback, invalid reference fallback, route/prepared mismatch fallback, and prepared-owned target-addressing or materialization output.
- Required non-regression proof: no x86 wrapper behavior change and no expected-string or prepared printer/debug output churn.
- Broader validation is required if a shared Route 3 helper, public prepared header, wrapper path, printer/debug path, or expected-output surface is modified.
