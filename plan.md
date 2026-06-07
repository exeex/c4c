# AArch64 Dispatch Prepared Producer Contract Surface

Status: Active
Source Idea: ideas/open/116_aarch64_dispatch_prepared_producer_contract_surface.md

## Purpose

Move or expose the remaining prepared edge-publication, current-block producer,
and join-routing facts that AArch64 dispatch still shapes locally before
edge-copy and publication materialization.

## Goal

Make AArch64 dispatch consume shared prepared producer/publication facts where
the facts are target-neutral, while keeping target-local AArch64 emission and
register policy in the AArch64 codegen layer.

## Core Rule

Do not claim progress by renaming local AArch64 routing logic, weakening tests,
or proving only one named testcase. Every implementation step must either
consume a shared prepared fact, expose a missing shared fact with proof, or
document with evidence why a remaining decision is genuinely target-local.

## Read First

- `ideas/open/116_aarch64_dispatch_prepared_producer_contract_surface.md`
- `src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_producers.cpp`
- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp`
- Existing shared prepared/BIR prealloc query owners under
  `src/backend/mir/`
- Existing backend dump or route tests covering prepared publication,
  parallel-copy, same-block producer, select-chain, or current-block join facts

## Current Scope

Primary targets:

- edge-publication producer/source context helpers in
  `dispatch_edge_copies.cpp`
- source memory/register publication helpers in `dispatch_edge_copies.cpp`
- `lower_predecessor_select_parallel_copy_sources`
- current-block join prepared-query routing and incoming source helpers in
  `dispatch_producers.cpp`
- same-block/select-chain producer wrappers and global-load select-chain checks
  in `dispatch_producers.cpp`
- publication register clobber/read checks in `dispatch_producers.cpp`

Supporting consumers:

- `dispatch.cpp`
- `dispatch_value_materialization.cpp`

## Non-Goals

- Do not perform mechanical file moves, translation-unit deletion, or
  line-count contraction before the shared contract is explicit.
- Do not reopen idea 47's old prepared-authority repair unless current code
  proves a new uncovered gap.
- Do not rebuild idea 64's closed join parallel-copy query under a new helper
  name.
- Do not move AArch64 instruction spelling, register allocation policy, memory
  operand emission, branch emission, or target-specific hazard policy into
  shared prepared code.
- Do not broaden this into dispatch phoenix work or unrelated cleanup in
  `dispatch.cpp`.

## Working Model

- Shared prepared/BIR prealloc code should own target-neutral facts about edge
  publication, move bundles, same-block producers, select chains, current-block
  joins, and publication sources.
- AArch64 dispatch should own target emission, machine-register hazard checks
  that genuinely require physical register knowledge, and glue that consumes
  prepared facts.
- Existing AArch64 helper names are evidence to inspect, not proof that the
  helper's responsibility belongs in AArch64.

## Execution Rules

- Start each implementation packet by characterizing the helper family and the
  shared facts it already consumes.
- Prefer adding or tightening dump/test visibility before deleting local
  routing authority.
- Keep source-idea edits out of routine execution; record packet progress and
  proof in `todo.md`.
- When a shared query is added or clarified, use focused coverage that shows
  the prepared fact directly or through the narrowest route test.
- Preserve behavior for `dispatch.cpp` and `dispatch_value_materialization.cpp`
  consumers; they must not gain new producer-discovery logic.
- Treat predecessor rescans, BIR-name matching, same-block named-case matching,
  or narrow select-chain special cases as route drift unless explicitly
  justified by a target-local emission need.

## Step 1: Characterize Dispatch Producer Contract Residue

Goal: Build a current map of which named AArch64 helpers own target-neutral
producer/publication routing and which already consume shared prepared facts.

Primary targets:

- `dispatch_edge_copies.cpp`
- `dispatch_producers.cpp`
- shared prepared/BIR prealloc query owners under `src/backend/mir/`

Actions:

- Inspect the helper call graph for edge-publication producer context, source
  producer context, current-block join routing, same-block/select-chain producer
  wrappers, and publication clobber/read checks.
- Identify existing shared prepared queries used by those helpers.
- Identify missing or too-private shared facts that force AArch64 to shape
  routing locally.
- Select the narrow proof commands and tests that cover at least one
  edge-publication/parallel-copy path and one current-block producer or
  join-routing path.

Completion check:

- `todo.md` contains a helper-to-contract map, named candidate shared facts,
  and supervisor-ready proof recommendations for the first implementation
  packet.

## Step 2: Expose Edge-Publication And Parallel-Copy Prepared Facts

Goal: Move or expose target-neutral edge-publication and parallel-copy producer
facts so `dispatch_edge_copies.cpp` consumes prepared authority instead of
reshaping it locally.

Primary targets:

- edge-publication producer/source context helpers in
  `dispatch_edge_copies.cpp`
- `lower_predecessor_select_parallel_copy_sources`
- shared prepared edge-publication or move-bundle query owners

Actions:

- Add or clarify shared query APIs for target-neutral edge-publication producer
  context, source producer context, publication source memory/register facts,
  or parallel-copy move-source matching where Step 1 proves AArch64 owns the
  fact locally.
- Update AArch64 edge-copy lowering to consume those prepared facts.
- Keep AArch64-only emission and physical register hazard details local.
- Add or tighten dump/test coverage for the exposed prepared facts before
  removing local routing authority.

Completion check:

- Edge-copy lowering no longer owns target-neutral prepared routing for the
  migrated helper family, and proof covers an edge-publication or
  parallel-copy path that previously depended on local AArch64 shape.

## Step 3: Expose Current-Block Join And Producer Routing Facts

Goal: Move or expose current-block join/source, same-block producer, and
select-chain producer facts so `dispatch_producers.cpp` consumes shared
prepared authority.

Primary targets:

- current-block join prepared-query routing helpers in
  `dispatch_producers.cpp`
- same-block and select-chain producer wrappers in `dispatch_producers.cpp`
- shared prepared current-block, same-block, and select-chain query owners

Actions:

- Add or clarify shared query APIs for current-block join incoming expression,
  current-block join source, same-block producer, select-chain producer, and
  direct-global-load select-chain facts where Step 1 proves they are
  target-neutral.
- Update AArch64 producer dispatch to consume those prepared facts.
- Keep fallback decisions and register hazard logic local only when they depend
  on target machine-register behavior.
- Add or tighten coverage for a current-block producer or join-routing path.

Completion check:

- `dispatch_producers.cpp` no longer owns target-neutral current-block
  producer or join-routing facts for the migrated helper family, and proof
  covers a current-block producer or join-routing path.

## Step 4: Audit Consumers And Register-Hazard Boundaries

Goal: Confirm the remaining AArch64 dispatch code is limited to emission,
target-local hazard checks, and glue that consumes prepared facts.

Primary targets:

- `dispatch_edge_copies.cpp`
- `dispatch_producers.cpp`
- `dispatch.cpp`
- `dispatch_value_materialization.cpp`

Actions:

- Re-audit publication register clobber/read checks after Steps 2 and 3.
- Document any keep-local hazard decision with concrete machine-register
  evidence in `todo.md`.
- Confirm `dispatch.cpp` and `dispatch_value_materialization.cpp` continue to
  consume equivalent prepared facts and did not gain new producer-discovery
  logic.
- Remove obsolete local helper wrappers only after their shared replacement is
  proved.

Completion check:

- `todo.md` contains a final consumer/hazard boundary map and no remaining
  renamed local shared-authority helper is being claimed as progress.

## Step 5: Final Proof And Closure Package

Goal: Produce the validation package needed for supervisor review and possible
source-idea closure.

Primary targets:

- `todo.md`
- `test_before.log`
- `test_after.log`

Actions:

- Run the supervisor-selected backend/AArch64 subset covering edge-copy,
  publication, dispatch-route, and current-block producer/join routing.
- Escalate to broader validation if shared query semantics changed outside the
  AArch64 dispatch consumers.
- Summarize migrated shared facts, remaining keep-local decisions, test/dump
  evidence, and any deferred follow-up.
- Confirm no supported-path expectations were weakened and no route relies on
  named-case shortcuts.

Completion check:

- The supervisor has a concise closure package showing that the source idea's
  acceptance criteria are satisfied or listing exact blockers that remain.
