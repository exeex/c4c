# Common Current-Block Join Query Exposure Runbook

Status: Active
Source Idea: ideas/open/729_common_current_block_join_query_exposure.md
Supersedes: exhausted consumer runbook for idea 709, which remains open and parked

## Purpose

Repair the query boundary that currently forces AArch64 to build executable
current-block join routing arrays before consumer retirement can finish.

## Goal

Provide direct typed access to existing attached current-block join authority,
remove the target-built routing array, and hand the bounded retirement route
back to idea 709.

## Core Rule

Expose existing prepared authority through its common owner. Do not change the
authoritative fact set or reproduce lookup reasoning in target materializers.

## Read First

- `ideas/open/729_common_current_block_join_query_exposure.md`
- `ideas/open/709_aarch64_named_handoff_materializer_cleanup.md`
- `ideas/closed/716_prealloc_current_block_routing_authority_closure.md`
- `src/backend/mir/aarch64/codegen/dispatch_producers.hpp`
- `src/backend/mir/aarch64/codegen/dispatch_producers.cpp`
- the common prepared current-block routing query declarations and owner
  attachment surfaces reached from those files

## Current Scope

- The attached current-block join consumption query contract.
- Incoming-expression and source roles currently cached by
  `CurrentBlockJoinPreparedQueryRouting`.
- Bounded AArch64 callers required to prove and delete that routing array.

## Non-Goals

- No change to current-block authority production or Route 5 diagnostic status.
- No broad lookup, publication, BIR, CFG, or other-target migration.
- No unrelated idea 709 retirement cleanup.
- No expectation weakening or testcase-shaped routing rule.

## Execution Rules

- Preserve the complete prepared authority established by idea 716: owner
  attachment, stable block/value identity, semantic role, and fail-closed
  disagreement or incompleteness.
- Keep common queries target-neutral and target callers limited to typed inputs
  plus instruction realization.
- Delete the routing array only after focused common and affected AArch64 proof
  demonstrates equivalent supported behavior and fail-closed negatives.
- Stop for lifecycle review if direct consumption requires changing the
  authoritative fact set rather than exposing it.

## Ordered Steps

### Step 1: Establish the direct common query seam

Goal: expose the already attached current-block join consumption authority
without a target-built routing cache.

Primary targets: the common prepared query declaration/implementation owner and
the smallest affected query contract tests.

Actions:

- Trace the existing private query to the authoritative prepared lookup and
  select the lowest target-neutral declaration surface.
- Define direct typed incoming-expression and source consumption queries using
  stable block/value/role inputs already owned by the prepared contract.
- Preserve explicit fail-closed results for missing attachment, stale or
  incomplete identity, ambiguity, disagreement, and unsupported roles.
- Add focused positive and negative proof across more than one instruction
  shape; do not use Route 5 or instruction position as authority.

Completion check:

- Common callers can query the attached relation directly, focused contract
  proof is green, and no target policy or new fact production entered the
  common query.

### Step 2: Remove the AArch64 routing-array reconstruction

Goal: migrate the bounded AArch64 current-block join callers to the direct
common query and delete the executable routing cache.

Primary targets:

- `src/backend/mir/aarch64/codegen/dispatch_producers.hpp`
- `src/backend/mir/aarch64/codegen/dispatch_producers.cpp`
- directly implicated callers in `src/backend/mir/aarch64/codegen/dispatch.cpp`

Actions:

- Replace `CurrentBlockJoinPreparedQueryRouting` parameters and callers with
  direct typed common-query consumption.
- Delete the builder, boolean arrays, and private duplicate query reasoning
  after their final executable consumers disappear.
- Preserve target-local instruction behavior and fail closed when common
  authority is unavailable or inconsistent.
- Prove affected AArch64 current-block/join positives and nearby fail-closed
  negatives without changing expectations.

Completion check:

- The routing struct and builder are gone, no AArch64 instruction scan caches
  executable join answers, and focused common plus AArch64 proof is green.

### Step 3: Prove the boundary and hand back idea 709

Goal: establish that the query-contract initiative is complete and the parked
consumer retirement route can resume without further scope expansion.

Actions:

- Search the affected common and AArch64 surfaces for renamed routing arrays,
  copied prepared lookup reasoning, Route 5 authority, or expectation changes.
- Run the supervisor-selected broader backend regression comparison.
- Review the complete slice against the source idea reject signals.
- Record the durable handback and reactivate idea 709 at its retirement step;
  do not absorb its locally replaceable address-materialization cleanup here.

Completion check:

- Direct common authority is the only executable query path, broader proof has
  no new failures, and lifecycle state can switch back to idea 709 with its
  remaining retirement work explicit.
