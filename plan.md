# Plan: AArch64 Calls After-Call Result Value Local Owner

Status: Active
Source Idea: ideas/open/120_aarch64_calls_after_call_result_value_local_owner.md

## Purpose

Extract a bounded AArch64-local owner for after-call result and value lowering.
The owner consumes prepared result plans, after-call ABI bindings, destination
homes, and f128 carrier facts without rediscovering closed shared authority.

## Goal

Move after-call result movement and value-store lowering into a clear
target-local owner while preserving prepared/shared ownership from ideas 93 and
94.

## Core Rule

Do not recompute result destination homes, stack frame slots, ABI result
classification, or f128 carrier choices. AArch64 code may only translate
prepared result authority into target register views, memory operands, and
call-boundary machine records.

## Read First

- `ideas/open/120_aarch64_calls_after_call_result_value_local_owner.md`
- `src/backend/mir/aarch64/codegen/calls.cpp`
- `src/backend/mir/aarch64/codegen/calls.hpp`, if local declarations are needed
- Existing backend/AArch64 tests covering integer, FP, f128/vector-carrier, and
  frame-slot result paths

## Current Targets

- `lower_after_call_move` and nearby after-call result/value helper cluster in
  `src/backend/mir/aarch64/codegen/calls.cpp`
- Prepared/shared inputs:
  - prepared result plans and result lanes
  - after-call ABI bindings
  - prepared destination value homes
  - prepared f128 carrier facts
  - frame-slot destination facts
- AArch64-local outputs:
  - GP, FP, VREG, and f128 register views
  - q/f128 result rendering
  - frame-slot memory operands
  - call-boundary move records
  - frame-slot store records for result values

## Non-Goals

- Do not reopen destination home or stack frame-slot ownership from idea 93.
- Do not reopen f128 carrier selection or q-register lookup ownership from idea
  94.
- Do not move target-specific ABI result register spelling, scalar/FPR/VREG view
  choice, q/f128 rendering, or frame-slot memory operand spelling into shared
  BIR/prealloc.
- Do not change shared call result planning semantics beyond narrow consumer
  interface needs.
- Do not perform unrelated calls lowering cleanup or broad file splitting.

## Working Model

Prepared/shared code owns result classification, result lane binding,
destination homes, stack storage facts, and f128 carrier facts.

The AArch64-local owner owns only conversion from those prepared facts into
target register views, frame-slot memory spelling, and selected machine records
after the call.

## Execution Rules

- Keep source idea edits unnecessary unless durable intent changes.
- Prefer small helper extraction before moving code into a new file.
- Preserve behavior first; semantic changes are out of scope unless needed to
  maintain the prepared-input contract.
- Use prepared facts directly. If a needed fact is unavailable, stop and record
  the missing prepared authority rather than adding local rediscovery.
- Treat expectation downgrades, unsupported-path rewrites, and named-case-only
  shortcuts as route failures.
- For code-changing steps, run a fresh build before claiming readiness.
- Use focused backend/AArch64 proof for every implementation packet. Escalate
  to broader backend proof if shared prepared result structures are touched.

## Ordered Steps

### Step 1: Characterize The Existing After-Call Result Cluster

Goal: map the current `lower_after_call_move` cluster and its prepared inputs
before making implementation changes.

Primary target: `src/backend/mir/aarch64/codegen/calls.cpp`

Actions:

- Inspect `lower_after_call_move` and direct helper/caller relationships.
- Identify which paths consume prepared result lanes, after-call ABI bindings,
  destination homes, frame-slot facts, and f128 carrier facts.
- Identify AArch64-local responsibilities that must stay local: result-register
  parsing, register-view conversion, q/f128 rendering, memory operand spelling,
  and call-boundary or frame-slot store record construction.
- List focused tests or dumps that cover scalar integer, scalar FP,
  f128/vector-carrier, and stack/frame-slot result paths.

Completion check:

- `todo.md` records the cluster map, local-owner boundary, available proof
  subset, and any missing coverage before code movement starts.

### Step 2: Extract Local Owner Surface In Place

Goal: introduce a bounded AArch64-local owner surface while keeping behavior
unchanged.

Primary target: `src/backend/mir/aarch64/codegen/calls.cpp`

Actions:

- Factor after-call result/value translation into named local helpers or a local
  owner object that accepts prepared inputs explicitly.
- Keep prepared result plans, ABI bindings, destination homes, and f128 carrier
  facts as inputs rather than recomputing them inside the owner.
- Keep target-specific register-view conversion, q/f128 rendering, memory
  operand spelling, frame-slot store records, and call-boundary record
  construction local to AArch64.
- Avoid moving unrelated calls lowering paths.

Completion check:

- The code builds, focused after-call backend/AArch64 proof passes, and the old
  authority is not merely hidden behind renamed helpers.

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
- If split, expose only the prepared-input interface needed by after-call result
  movement.
- Keep shared prepared contracts unchanged unless a compile error proves a
  narrow declaration adjustment is required.

Completion check:

- The file boundary is target-local, the build passes, and focused
  backend/AArch64 proof remains green.

### Step 4: Prove Neighboring After-Call Result Routes

Goal: prove the extraction across nearby after-call result shapes, not only one
fixture.

Primary target: focused backend/AArch64 tests and route dumps.

Actions:

- Run or add focused proof for scalar integer result movement.
- Run or add focused proof for scalar FP result movement.
- Run or add focused proof for f128 or vector-carrier result movement.
- Run or add focused proof for stack/frame-slot result storage.
- Confirm prepared destination homes and prepared f128 carrier facts are
  consumed while target register and memory spelling stays AArch64-local.

Completion check:

- Focused proof covers GP, FP, f128/vector-carrier, and frame-slot result paths.
  Any coverage gap is recorded in `todo.md` with a clear reason and next action.

### Step 5: Acceptance Review Package

Goal: decide whether idea 120 is complete under its source criteria.

Actions:

- Compare the final diff against the owner boundary and reviewer reject signals
  in the source idea.
- Confirm there is no destination-home recomputation, stack frame-slot
  reopening, or f128 carrier rediscovery.
- Confirm target-specific register spelling, register-view policy, q/f128
  rendering, and memory operand spelling remain AArch64-local.
- Run the supervisor-selected close proof or prepare exact validation
  recommendations for the supervisor.

Completion check:

- `todo.md` records whether the source idea appears closure-ready, what proof
  was run, and any residual follow-up needed before lifecycle closure.
