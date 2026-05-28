# AArch64 Shared Select-Chain Dependency Authority Runbook

Status: Active
Source Idea: ideas/open/63_aarch64_shared_select_chain_dependency_authority.md

## Purpose

Move direct-global and select-chain dependency discovery into shared prepared
select-chain or call-plan authority, while relocating AArch64 select
materialization to a precise local owner instead of broad dispatch-family code.

## Goal

AArch64 select-chain consumers should use shared/prepared dependency facts for
direct-global and select-chain source identity, while AArch64 keeps only
target-local materialization and call-argument emission details.

## Core Rule

Do not preserve AArch64-local direct-global or select-chain traversal as the
semantic authority after shared/prepared facts can answer the dependency
question. AArch64 may own register spelling, call-argument sequencing, scratch
hazards, and target instruction emission only after the shared/prepared fact is
known.

## Read First

- `ideas/open/63_aarch64_shared_select_chain_dependency_authority.md`
- `src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_producers.cpp`
- `src/backend/mir/aarch64/codegen/calls.cpp`
- AArch64 ALU and value-materialization select-chain callers.
- Shared prepared select-chain or `PreparedCallPlan` owners that can expose
  direct-global and select-chain dependency facts.

## Current Targets

- `emit_select_chain_value_to_register` in
  `src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp`.
- `materialize_direct_global_select_chain_call_argument` in
  `src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp`.
- `prepared_select_chain_contains_direct_global_load` and prepared select
  bridge fallback behavior in
  `src/backend/mir/aarch64/codegen/dispatch_producers.cpp`.
- Call-argument consumers in
  `src/backend/mir/aarch64/codegen/calls.cpp`.
- ALU and value-materialization callers that materialize select-chain values.

## Non-Goals

- Do not move AArch64 select emission, register spelling, instruction
  sequencing, or call-argument materialization into shared code.
- Do not solve same-block scalar recursion, edge fallback, or join-copy cache
  migration in this runbook.
- Do not add select-chain-shaped special cases for one direct-global pattern.
- Do not bulk move select handling into `dispatch.cpp`.
- Do not weaken supported-path expectations or mark select-chain cases
  unsupported to hide missing prepared facts.

## Working Model

The current route has multiple AArch64 consumers that can rediscover
direct-global and select-chain dependencies locally. The semantic dependency
question should be answered once by shared prepared select-chain or call-plan
authority, then consumed by target-local AArch64 emission helpers. This runbook
separates the fact owner from the target owner and avoids replacing one broad
dispatch fallback with another renamed traversal.

Missing shared/prepared facts must be recorded explicitly in `todo.md` and
handled with deliberate fail-closed behavior or diagnostics. They must not be
covered by new AArch64 select-chain matching shortcuts.

## Execution Rules

- Start with an inventory of every select-chain dependency traversal before
  changing behavior.
- Separate semantic dependency discovery from AArch64 target materialization in
  every edited helper.
- Prefer one shared prepared select-chain or call-plan query contract that
  covers direct-global dependencies for call arguments, ALU materialization,
  value materialization, and publication through select chains.
- Keep target-local register spelling, scratch handling, call-argument
  instruction sequencing, and materialization details in precise AArch64
  owners.
- Do not add named-case filters, select-chain-shaped shortcuts, expectation
  downgrades, or raw BIR scans as the new authority.
- Record any missing shared fact gap in `todo.md`; do not fill it with a new
  AArch64 fallback.
- For code-changing steps, run at least a fresh build and the focused AArch64
  backend subset chosen by the supervisor. Escalate proof if shared query
  contracts, call-plan structures, or public helper headers change.

## Ordered Steps

### Step 1: Inventory Select-Chain Dependency Callers

Goal: classify every AArch64 direct-global and select-chain dependency lookup
by current authority, required shared fact, and target-local emission
responsibility.

Primary targets:
`src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp`,
`src/backend/mir/aarch64/codegen/dispatch_producers.cpp`,
and `src/backend/mir/aarch64/codegen/calls.cpp`.

Actions:
- Find callers of `emit_select_chain_value_to_register`,
  `materialize_direct_global_select_chain_call_argument`, and
  `prepared_select_chain_contains_direct_global_load`.
- Identify ALU and value-materialization select-chain consumers outside the
  edge-copy path.
- Classify which callers need direct-global, select-chain bridge,
  call-argument, publication, or materialization dependency facts.
- Separate helpers that only emit AArch64 target instructions from helpers that
  rediscover semantic dependency identity.
- Identify existing shared prepared select-chain or `PreparedCallPlan` facts
  that already cover each dependency question.
- Record missing shared/prepared facts that must exist before local traversal
  can be removed.

Completion check:
- `todo.md` records the caller inventory, the shared owner candidate for each
  dependency fact, and any blocked fact gaps without implementation edits.

### Step 2: Define Shared Select-Chain Dependency Facts

Goal: expose direct-global and select-chain dependency identity through shared
prepared select-chain or call-plan authority.

Primary target:
the shared prepared select-chain or `PreparedCallPlan` owner identified in
Step 1.

Actions:
- Add or extend structured query APIs for direct-global select-chain
  dependencies required by AArch64 consumers.
- Ensure `PreparedCallPlan` or the shared select-chain owner covers existing
  call-argument cases before local AArch64 traversal is removed.
- Keep query results descriptive enough for AArch64 to materialize values and
  call arguments without rediscovering dependency identity.
- Preserve existing prepared authority semantics for missing, ambiguous, or
  unsupported select-chain facts.
- Add focused shared-level coverage or assertions where the repository has an
  established test surface for prepared select-chain or call-plan facts.

Completion check:
- Shared/prepared APIs can answer the in-scope direct-global and select-chain
  dependency questions for current AArch64 callers, and missing facts have
  deliberate fail-closed or diagnostic behavior.

### Step 3: Migrate Call-Argument Select-Chain Consumers

Goal: make direct-global select-chain call-argument materialization consume
shared/prepared facts instead of AArch64-local traversal.

Primary targets:
`materialize_direct_global_select_chain_call_argument` and call-argument
consumers in `src/backend/mir/aarch64/codegen/calls.cpp`.

Actions:
- Replace local dependency traversal in call-argument paths with the
  shared/prepared query API from Step 2.
- Keep call-argument register spelling, instruction order, and target emission
  in AArch64.
- Preserve behavior for covered call-argument select chains while routing
  missing facts to explicit fail-closed or diagnostic behavior.
- Avoid making calls depend on edge-copy internals or broad dispatch-family
  helpers for semantic dependency identity.

Completion check:
- Direct-global select-chain call arguments are covered by shared/prepared
  facts, and migrated call paths no longer rediscover the dependency through
  AArch64-local select-chain traversal.

### Step 4: Migrate ALU And Value-Materialization Select Consumers

Goal: route non-call select-chain materialization through shared/prepared
dependency facts while keeping target emission local.

Primary targets:
`emit_select_chain_value_to_register`, ALU select materialization, and
value-materialization select-chain callers.

Actions:
- Replace local dependency checks in ALU and value-materialization callers with
  the shared/prepared query API from Step 2.
- Keep target-local register selection, scratch hazards, load/store spelling,
  and materialization instruction sequences in precise AArch64 owners.
- Remove prepared select bridge fallback behavior once all callers use the
  shared/prepared facts.
- Avoid widening dispatch-family ownership or moving target emission details to
  shared code.

Completion check:
- Non-edge select chains, ALU materialization, and value publication through
  select chains consume shared/prepared dependency facts without relying on
  local direct-global traversal.

### Step 5: Relocate AArch64 Select Materialization Ownership

Goal: move remaining target-local select materialization helpers out of broad
dispatch-family ownership into a precise AArch64 local owner.

Primary target:
the AArch64 local owner chosen for select materialization and call-argument
emission details.

Actions:
- Choose a precise local owner for select-chain value materialization and
  direct-global select-chain call-argument emission.
- Move only target-local emission details into that owner; keep semantic
  dependency facts in shared prepared select-chain or call-plan authority.
- Shrink public dispatch-family surfaces where call sites can use the precise
  owner directly.
- Preserve register spelling, scratch handling, call-argument sequencing, and
  instruction emission behavior.

Completion check:
- Broad dispatch-family code no longer owns select-chain materialization
  helpers except for necessary routing, and no semantic dependency discovery
  moved back into the new local target owner.

### Step 6: Prove Select-Chain Coverage And Ownership Split

Goal: validate shared select-chain dependency authority and target-local
materialization preservation without overfitting one select-chain testcase.

Primary target:
focused AArch64 backend tests and any shared prepared select-chain or call-plan
tests touched by the migration.

Actions:
- Run a fresh build or compile proof selected by the supervisor.
- Run focused proof for non-edge select chains, direct-global select-chain call
  arguments, ALU select materialization, and value publication through select
  chains.
- Compare coverage with shared preallocation or publication-plan dependency
  facts where those are relevant to the migrated dependency owner.
- Inspect the diff for duplicated direct-global traversal, renamed fallback
  helpers, select-chain-shaped shortcuts, weakened expectations, unsupported
  downgrades, or target emission sequencing changes.
- Ask the supervisor to escalate to broader backend or regression-guard proof
  if shared query contracts, call-plan structures, public headers, or multiple
  backend buckets were changed.

Completion check:
- Build and focused proof are green, `todo.md` records exact commands and log
  names, and the diff contains no durable AArch64 direct-global or select-chain
  dependency traversal that replaces the old local discovery under a new helper
  name.
