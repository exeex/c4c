# Shared Select-Chain And Same-Block Dependency Queries Runbook

Status: Active
Source Idea: ideas/open/134_shared_select_chain_same_block_dependency_queries.md

## Purpose

Move remaining target-neutral select-chain and same-block dependency facts out
of AArch64-local helper queries and behind a shared prepared/prealloc query
surface.

## Goal

Replace concrete AArch64 rediscovery sites for select-chain direct-global
dependency and same-block scalar or constant materialization relationships with
shared queries while preserving AArch64 emission behavior.

## Core Rule

Generalize prepared fact relationships; do not move AArch64 materialization
emission, hazard policy, scratch choice, or instruction ordering into shared
code.

## Read First

- `ideas/open/134_shared_select_chain_same_block_dependency_queries.md`
- `src/backend/mir/aarch64/codegen/dispatch_producers.hpp`
- `src/backend/mir/aarch64/codegen/dispatch_producers.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp`

## Current Targets

- AArch64 select-chain direct-global dependency checks
- AArch64 same-block scalar or constant materialization checks
- The shared MIR, prepared, or prealloc query owner that already owns the
  underlying facts
- Backend coverage around nearby select-chain and same-block dependency forms

## Non-Goals

- Do not rewrite AArch64 instruction emission.
- Do not change register-read hazard handling, scratch selection, or final
  materialization order.
- Do not add named-case select-chain shortcuts.
- Do not add direct-global special cases that bypass the prepared fact model.
- Do not broadly reshape dispatch-family files for file-count cleanup.
- Do not weaken expectations or mark supported paths unsupported.

## Working Model

- Shared code should answer target-neutral relationship questions such as
  whether a prepared select-chain value depends on a direct global or whether a
  value can be materialized from same-block scalar/constant facts.
- AArch64 code should consume those answers and keep target-specific register,
  hazard, and emission decisions local.
- Public dispatch producer surface may shrink only when call sites move to a
  clearer shared owner.

## Execution Rules

- Prefer one concrete rediscovery site per step; avoid broad cleanup while the
  query shape is still being proven.
- When adding shared query APIs, name them for the prepared relationship they
  expose, not for an AArch64 helper shape.
- Prove representative nearby cases; a single named target testcase is not
  enough evidence.
- Keep matching `test_before.log` and `test_after.log` for the backend subset
  because shared query code is in scope.
- For code-changing steps, run `cmake --build --preset default` before the
  delegated proof command.

## Steps

### Step 1: Inventory Local Dependency Queries

Goal: identify the exact AArch64-local checks that rediscover select-chain
direct-global and same-block scalar/constant materialization relationships.

Primary targets:
- `src/backend/mir/aarch64/codegen/dispatch_producers.hpp`
- `src/backend/mir/aarch64/codegen/dispatch_producers.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp`

Actions:
- Inspect the producer and value-materialization helpers named around
  select-chain direct-global dependencies.
- Inspect same-block scalar or constant materialization checks and their
  prepared fact inputs.
- Identify the existing shared MIR, prepared, or prealloc owner that should
  expose each relationship.
- Record candidate call sites and nearby tests in `todo.md`; do not edit source
  code during inventory unless delegated as part of the same executor packet.

Completion check:
- `todo.md` names the concrete helper or call site to replace first, the
  proposed shared owner, and the narrow proof subset the supervisor should use.

### Step 2: Add The First Shared Prepared Query

Goal: expose one selected target-neutral prepared relationship through the
shared owner and replace the matching AArch64-local rediscovery site.

Primary targets:
- The selected shared query header/source
- The AArch64 helper or call site identified in Step 1

Actions:
- Add a shared API that describes the prepared relationship directly.
- Route the selected AArch64 site through the shared API.
- Keep AArch64 register-read hazard behavior, scratch choice, and final
  materialization order unchanged.
- Remove or narrow obsolete local helper surface only when all current callers
  have moved to the shared owner.

Completion check:
- The selected local rediscovery site is gone or reduced to target-specific
  adaptation.
- `cmake --build --preset default` succeeds.
- The delegated backend subset covers the changed relationship and does not
  rely on expectation weakening.

### Step 3: Cover The Companion Same-Block Or Select-Chain Relationship

Goal: apply the same shared-query pattern to the remaining in-scope
select-chain or same-block materialization relationship.

Primary targets:
- `src/backend/mir/aarch64/codegen/dispatch_producers.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp`
- The selected shared query files

Actions:
- Reuse the shared owner pattern from Step 2 when it fits the underlying fact
  model.
- Avoid cloning an AArch64 helper shape into shared code.
- Keep any AArch64-only emission or hazard policy local.
- Add or update focused coverage only when it demonstrates a nearby
  relationship form, not a named-case shortcut.

Completion check:
- The remaining in-scope rediscovery site uses a shared query or is documented
  in `todo.md` as intentionally target-specific with evidence.
- Build proof succeeds and the delegated backend subset covers nearby forms.

### Step 4: Run Backend Regression Guard

Goal: prove the shared-query route did not regress backend behavior.

Actions:
- Ensure `test_before.log` and `test_after.log` are produced with matching
  commands.
- Run:
  - `cmake --build --preset default`
  - `ctest --test-dir build -R '^backend_' --output-on-failure`
- Compare failures and investigate any new backend regressions before closure.

Completion check:
- Matching backend before/after logs exist.
- No new backend regression is accepted as progress.
- `todo.md` records the final proof command and result.

## Acceptance Check

- Local select-chain or same-block dependency checks are replaced by shared
  prepared fact queries.
- AArch64 materialization and register-read hazard behavior are preserved.
- Proof covers representative nearby cases.
- Public dispatch producer surface is narrowed only where a clearer shared
  owner replaces local callers.
