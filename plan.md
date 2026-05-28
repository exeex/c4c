# AArch64 Local Dispatch Block Route Runbook

Status: Active
Source Idea: ideas/open/66_aarch64_local_dispatch_block_route.md

## Purpose

Preserve the local AArch64 prepared-block dispatch route while narrowing the
remaining public block entry points, diagnostics, hook wiring, and sequencing
after the producer, publication, and value-materialization helper splits.

## Goal

Keep AArch64 block traversal and dispatch sequencing local, but make the route
explicitly limited to traversal, diagnostics, hook wiring, before-return
publication sequencing, branch-fusion integration, and final machine
instruction ordering.

## Core Rule

Do not move AArch64 block traversal or target diagnostics to shared code, and
do not take semantic producer, publication, or value-materialization authority
back into `dispatch.cpp`.

## Read First

- `ideas/open/66_aarch64_local_dispatch_block_route.md`
- `src/backend/mir/aarch64/codegen/dispatch.hpp`
- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- Current precise owners introduced by prior producer, publication, value
  materialization, global, memory, ALU, and helper relocation work.
- Focused AArch64 dispatch tests covering unsupported diagnostics,
  before-return publication, branch-fusion hooks, and prepared block lowering.

## Current Targets

- Public block entry points:
  `make_block_lowering_context` and `dispatch_prepared_block`.
- Routing and diagnostics:
  `classify_instruction`, unsupported instruction/terminator diagnostics,
  BIR block/context lookup, and `make_bir_machine_instruction`.
- Local sequencing:
  hook wiring, before-return publication sequencing, branch-fusion integration,
  and final machine-instruction ordering.
- Remaining dispatch-family helper surfaces that are route-level adapters
  rather than semantic producer/publication/value-materialization authority.

## Non-Goals

- Do not move AArch64 block traversal or target diagnostics to shared code.
- Do not reintroduce semantic producer or publication decisions into
  `dispatch.cpp`.
- Do not weaken unsupported diagnostics to accommodate moved helpers.
- Do not solve all dispatch-family contraction in one broad change.
- Do not bundle idea 67's local-slot offset evidence probe into this route.

## Working Model

The core block dispatch route is intentionally AArch64-local. It owns the
target traversal loop, diagnostics, hook ordering, before-return publication,
branch-fusion hook integration, and final machine-instruction sequencing. After
prior helper splits, this plan should make that keep-local boundary precise
without turning `dispatch.cpp` back into a broad semantic owner.

The route should preserve behavior first. Any narrowing should happen through
small slices that separate route-level adapters from semantic helper authority.
If a helper still belongs to a producer, publication, value-materialization, or
target-emission owner, move or call it there; if it is required for route
sequencing, keep it local and document why in `todo.md`.

## Execution Rules

- Start with an inventory of current public block entry points, route-local
  helpers, diagnostics, hooks, and sequencing responsibilities.
- Classify each remaining helper as route-local, precise-owner candidate, or
  out-of-scope semantic authority.
- Keep unsupported diagnostics durable and specific.
- Preserve before-return publication ordering, branch-fusion hook ordering, and
  final machine-instruction ordering.
- Prefer small public-surface narrowing packets over broad rewrites.
- Run `git diff --check` for code-changing packets.
- Run the supervisor-selected backend/AArch64 proof for code-changing packets;
  escalate if public block entry points or sequencing contracts change.

## Ordered Steps

### Step 1: Inventory Local Dispatch Route Responsibilities

Goal: classify the current `dispatch.hpp`/`dispatch.cpp` surface by route-local
responsibility, precise-owner candidate, and out-of-scope semantic authority.

Primary targets:
`src/backend/mir/aarch64/codegen/dispatch.hpp` and
`src/backend/mir/aarch64/codegen/dispatch.cpp`.

Actions:
- List public entry points and their call sites.
- Inventory `classify_instruction`, unsupported diagnostics, BIR
  block/context lookup, `make_bir_machine_instruction`, hook wiring,
  before-return publication sequencing, branch-fusion integration, and final
  instruction append/order behavior.
- Identify remaining helpers that still appear broader than route-local
  dispatch ownership.
- Identify proof coverage for traversal/routing, diagnostics,
  before-return publication, branch-fusion hooks, and representative prepared
  block lowering.
- Record the inventory and first coherent narrowing packet in `todo.md`
  without implementation edits.

Completion check:
- `todo.md` records route-local responsibilities, any precise-owner candidates,
  public-surface notes, sequencing risks, and proof needs.

### Step 2: Narrow Public Block Entry Points

Goal: make the public block dispatch surface explicit and minimal without
changing traversal behavior.

Primary targets:
`dispatch.hpp`, `dispatch.cpp`, and call sites of
`make_block_lowering_context` and `dispatch_prepared_block`.

Actions:
- Inspect whether current public block entry points are all still needed by
  callers.
- Remove or restrict declarations only where callers can use a precise owner or
  local adapter directly.
- Preserve `BlockLoweringContext` construction semantics and prepared-block
  dispatch behavior.
- Avoid moving block traversal or target diagnostics to shared code.

Completion check:
- Public block entry points are smaller or explicitly justified, and focused
  proof preserves representative prepared-block lowering.

### Step 3: Keep Diagnostics Durable While Narrowing Route Helpers

Goal: preserve unsupported instruction and terminator diagnostics while
separating diagnostic helpers from unrelated route work.

Primary targets:
`classify_instruction`, unsupported diagnostic helpers, and diagnostic call
sites in `dispatch.cpp`.

Actions:
- Confirm unsupported diagnostics still include stable function/block,
  instruction index, family, and message information.
- Narrow helper visibility if possible without weakening diagnostic behavior.
- Do not suppress diagnostics to accommodate moved helpers.
- Preserve route behavior for unsupported instruction and terminator cases.

Completion check:
- Unsupported diagnostics remain durable and focused proof covers unsupported
  instruction and terminator paths.

### Step 4: Preserve Hook Wiring And Sequencing Boundaries

Goal: keep before-return publication, branch-fusion integration, hook routing,
and final machine-instruction sequencing local and explicit.

Primary targets:
hook wiring in `dispatch.cpp`, before-return publication sequencing,
branch-fusion hook integration, and final instruction append/order logic.

Actions:
- Identify the local ordering contract around pre-instruction hooks,
  instruction lowering, branch-fusion hooks, before-return publication, and
  terminator emission.
- Narrow helper boundaries only when the ordering remains visibly local.
- Do not move branch-fusion sequencing or before-return publication ordering to
  shared code.
- Record any ordering-sensitive helper that must stay in `dispatch.cpp`.

Completion check:
- Hook and sequencing responsibilities are explicit, and proof covers
  before-return publication, branch-fusion hooks, and final instruction order.

### Step 5: Remove Residual Semantic Drift From Block Dispatch

Goal: ensure `dispatch.cpp` does not regain producer, publication, or
value-materialization authority after prior helper splits.

Primary targets:
remaining helper calls and includes in `dispatch.cpp` plus public
dispatch-family headers.

Actions:
- Scan for producer, publication, value-materialization, prepared query, or
  target-emission logic that should live in precise owners.
- Move or route through precise owners only when behavior can be preserved in a
  small packet.
- Keep route-level adapters local when they only orchestrate sequencing.
- Reject forwarding-only helper renames that leave broad authority unchanged.

Completion check:
- `dispatch.cpp` remains a local route owner rather than a semantic producer,
  publication, or value-materialization owner.

### Step 6: Prove Local Dispatch Route Preservation

Goal: validate that the local block route is precise and behavior-preserving.

Primary targets:
focused AArch64/backend tests and route-quality scans.

Actions:
- Run the supervisor-delegated proof command after the final code-changing
  packet.
- Run `git diff --check`.
- Scan for semantic producer/publication/value-materialization authority in
  `dispatch.cpp`.
- Confirm proof covers block traversal/routing, unsupported instruction and
  terminator diagnostics, before-return publication, branch-fusion hooks, and
  representative prepared-block lowering.
- Escalate validation if public block entry points, diagnostics, or sequencing
  contracts changed broadly.

Completion check:
- Backend proof and route-quality scans are green, diagnostics are not
  weakened, and the diff preserves the local route while avoiding broad
  semantic authority in `dispatch.cpp`.
