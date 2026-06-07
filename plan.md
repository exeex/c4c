# Plan: AArch64 Calls Before-Call Move Bundle Local Owner

Status: Active
Source Idea: ideas/open/119_aarch64_calls_before_call_move_bundle_local_owner.md

## Purpose

Extract a bounded AArch64-local owner for before-call move bundle lowering.
The owner consumes already-prepared call, move, source, ABI, immediate, f128,
stack-slot, and outgoing-stack-area facts without rediscovering them.

## Goal

Move before-call argument move lowering into a clear target-local owner while
preserving prepared/shared authority from ideas 93, 94, 95, and 114.

## Core Rule

Do not reselect, recompute, or rediscover prepared facts. AArch64 code may only
translate prepared authority into target call-boundary records and machine
instructions.

## Read First

- `ideas/open/119_aarch64_calls_before_call_move_bundle_local_owner.md`
- `src/backend/mir/aarch64/codegen/calls.cpp`
- `src/backend/mir/aarch64/codegen/calls.hpp`, if local declarations are needed
- Existing backend/AArch64 tests that cover before-call register, stack,
  immediate, FP, f128, and outgoing-stack-area argument moves

## Current Targets

- `lower_before_call_move` and nearby helper cluster in
  `src/backend/mir/aarch64/codegen/calls.cpp`
- Prepared inputs:
  - `PreparedCallPlan`
  - `PreparedMoveBundle`
  - `PreparedCallBoundaryEffectPlan`
  - `PreparedMoveResolution`
  - prepared value homes
  - prepared ABI bindings
  - prepared source selections
  - prepared f128 carriers
  - indexed immediate argument lookups
  - `PreparedCallPlan::outgoing_stack_argument_area`
- AArch64-local outputs:
  - register and memory move records
  - immediate and inline-asm materialization
  - q-register and f128 rendering
  - `x16` outgoing-stack-base addressing
  - target diagnostics for incomplete prepared authority

## Non-Goals

- Do not reopen stack homes, stack destinations, or stack-preserved source
  authority from idea 93.
- Do not reopen f128 carrier lookup or q-register transport authority from
  idea 94.
- Do not reopen immediate scalar argument publication from idea 95.
- Do not recompute outgoing stack area totals covered by idea 114.
- Do not move AArch64 ABI spelling, scratch registers, or machine-record
  emission into shared BIR/prealloc.
- Do not perform unrelated `calls.cpp` cleanup or broad file splitting.

## Working Model

Prepared/shared code owns call planning, source selection, destination homes,
ABI bindings, immediate publication, f128 carrier facts, stack-frame-slot
facts, and outgoing-stack-area authority.

The AArch64-local owner owns only conversion from those prepared facts into
target call-boundary move records and machine instructions for before-call
argument movement.

## Execution Rules

- Keep source idea edits unnecessary unless the durable intent changes.
- Prefer small helper extraction before moving code into a new file.
- Preserve behavior first; capability changes are out of scope unless needed
  to maintain the existing contract.
- Use prepared facts directly. If a needed fact is unavailable, stop and record
  the missing prepared authority rather than adding local rediscovery.
- Treat expectation downgrades, unsupported-path rewrites, and named-case-only
  shortcuts as route failures.
- For code-changing steps, run a fresh build before claiming readiness.
- Use focused backend/AArch64 proof for every implementation packet. Escalate
  to broader backend proof if shared prepared interfaces are touched.

## Ordered Steps

### Step 1: Characterize The Existing Before-Call Move Cluster

Goal: map the current `lower_before_call_move` cluster and its prepared inputs
before making implementation changes.

Primary target: `src/backend/mir/aarch64/codegen/calls.cpp`

Actions:

- Inspect `lower_before_call_move` and direct helper/caller relationships.
- Identify which code paths consume prepared move bundles, ABI bindings,
  prepared source selections, immediate argument lookups, f128 carriers, stack
  destinations, and outgoing-stack-area facts.
- Identify AArch64-local responsibilities that must stay local: `x16`
  addressing, register spelling, q-register rendering, inline-asm
  materialization, and target diagnostics.
- List the focused tests or dumps that cover before-call register, stack,
  immediate, FP/f128, and outgoing-stack-area routes.

Completion check:

- `todo.md` records the cluster map, local-owner boundary, available proof
  subset, and any missing coverage before code movement starts.

### Step 2: Extract Local Owner Surface In Place

Goal: introduce a bounded AArch64-local owner surface while keeping behavior
unchanged.

Primary target: `src/backend/mir/aarch64/codegen/calls.cpp`

Actions:

- Factor the before-call move translation into named local helpers or a local
  owner object that accepts prepared inputs explicitly.
- Keep prepared facts as inputs rather than recomputing them inside the owner.
- Keep target-specific machine-record construction, register spelling, memory
  operand spelling, and diagnostics local to AArch64.
- Avoid moving unrelated calls lowering paths.

Completion check:

- The code builds, focused before-call backend/AArch64 proof passes, and the
  old authority is not merely hidden behind renamed helpers.

### Step 3: Split File Boundary Only If The Owner Is Stable

Goal: move the local owner into a separate AArch64 calls helper file only if
that reduces coupling without expanding scope.

Primary targets:

- `src/backend/mir/aarch64/codegen/calls.cpp`
- a new or existing AArch64 calls helper file
- `src/backend/mir/aarch64/codegen/calls.hpp` or nearby local declarations, if
  needed

Actions:

- Decide whether the in-place owner is sufficiently isolated to justify a file
  split.
- If split, expose only the prepared-input interface needed by before-call
  movement.
- Keep shared prepared contracts unchanged unless a compile error proves a
  narrow declaration adjustment is required.

Completion check:

- The file boundary is target-local, the build passes, and focused
  backend/AArch64 proof remains green.

### Step 4: Prove Neighboring Before-Call Routes

Goal: prove the extraction across nearby before-call argument shapes, not only
one fixture.

Primary target: focused backend/AArch64 tests and route dumps.

Actions:

- Run or add focused proof for register arguments.
- Run or add focused proof for stack arguments and outgoing-stack-area
  reservation/address consumption.
- Run or add focused proof for immediate scalar arguments.
- Run or add focused proof for FP and f128/q-register argument movement.
- Confirm at least one route consumes `PreparedCallPlan::outgoing_stack_argument_area`
  while `x16` base emission stays AArch64-local.

Completion check:

- Focused proof covers register, stack, immediate, FP/f128, and outgoing-stack
  before-call routes. Any coverage gap is recorded in `todo.md` with a clear
  reason and next action.

### Step 5: Acceptance Review Package

Goal: decide whether idea 119 is complete under its source criteria.

Actions:

- Compare the final diff against the owner boundary and reviewer reject
  signals in the source idea.
- Confirm there is no stack-home reselecting, f128 carrier rediscovery,
  immediate-publication rediscovery, or outgoing-stack-area recomputation.
- Confirm the remaining AArch64 code is limited to local emission concerns.
- Run the supervisor-selected close proof or prepare exact validation
  recommendations for the supervisor.

Completion check:

- `todo.md` records whether the source idea appears closure-ready, what proof
  was run, and any residual follow-up needed before lifecycle closure.
