# AArch64 Pointer-Valued Subobject Address Publication Runbook

Status: Active
Source Idea: ideas/open/372_aarch64_pointer_valued_subobject_address_publication.md
Activated From: ideas/open/295_backend_regex_failure_family_inventory.md

## Purpose

Repair the focused `00163` pointer-valued address publication residual
selected by the backend inventory.

## Goal

Make scalar pointer locals consume the current address-valued assignment for
global object and subobject addresses before later dereferences.

## Core Rule

Repair pointer-local address publication generally. Do not special-case
`00163`, `bolshevic`, `b`, one struct field, one stack offset, one register,
or one emitted instruction sequence.

## Read First

- `ideas/open/372_aarch64_pointer_valued_subobject_address_publication.md`
- `todo.md`
- `test_after.log`
- `tests/c/external/c-testsuite/src/00163.c`
- generated `build/c_testsuite_aarch64_backend/src/00163.c.s`
- semantic/prepared BIR for `00163`
- closed idea notes for adjacent owners:
  - `ideas/closed/294_aarch64_pointer_derived_address_lvalue_lowering_authority.md`
  - `ideas/closed/355_aarch64_address_valued_memory_call_argument_publication.md`

## Current Scope

- scalar pointer-local assignment from `&global` or `&global.member`
- publication of that address value to the local pointer home/register
- dereference through the updated scalar pointer local
- representative proof for `c_testsuite_aarch64_backend_src_00163_c`

## Non-Goals

- Do not reopen or edit closed ideas 294 or 355.
- Do not broaden into external call return publication, selected array
  store/readback, aggregate initializer/layout, scalar FP, bit-field, or
  timeout buckets.
- Do not change expectations, unsupported classifications, runners, timeout
  policy, proof-log policy, or CTest registration.
- Do not implement under umbrella idea 295.

## Working Model

`00163` first stores a pointer to local `a` in `b`, then later assigns `b =
&(bolshevic.b)`. Generated AArch64 still dereferences the old pointer home for
`&a`, so the final output prints `42` instead of the global member value `34`.
The owner is scalar pointer-local publication of a current address-valued
assignment, not global struct field writeback or external call publication.

## Execution Rules

- Start from source, generated AArch64, and semantic/prepared records for
  `b = &(bolshevic.b)` and the final `*b`.
- Trace where the global member address value is materialized or lost before
  the local pointer home/dereference consumer.
- Compare against closed ideas 294 and 355 so the route does not repeat an
  archived boundary or reopen from counts alone.
- Add focused coverage before or with the repair.
- Preserve adjacent pointer-derived address/lvalue and address-valued
  call-argument behavior.

## Steps

### Step 1: Localize Pointer-Local Address Publication Gap

Goal: identify the first backend boundary where `b = &(bolshevic.b)` fails to
publish the current address value to the later `*b` consumer.

Primary target: generated `00163.c.s`, semantic/prepared BIR for `00163`, and
AArch64 pointer-local publication helpers.

Actions:

- Inspect generated AArch64 for the first `b = &a` assignment, the later
  `b = &(bolshevic.b)` assignment, and final `*b` load.
- Trace whether the global member address is present in semantic/prepared
  state and whether it reaches stack layout, MIR, and AArch64 lowering.
- Identify whether the stale `&a` value is retained in a stack home, register,
  or scalar state map.
- Decide the focused coverage shape needed before repair.

Completion check:

- `todo.md` names the first bad fact, owning backend boundary, and focused
  coverage requirement for pointer-local address publication.

### Step 2: Add Focused Pointer-Local Address Coverage

Goal: guard scalar pointer-local assignment from global/member addresses
independently of `00163`.

Primary target: backend tests or dump coverage for address-valued assignments
to scalar pointer locals followed by dereference.

Actions:

- Add coverage where a pointer local is assigned one address, then reassigned
  to a different global or member address.
- Assert the later dereference consumes the reassigned address value, not the
  stale prior pointer home.
- Keep the test semantic and not tied to `bolshevic`, one register, one stack
  offset, or one emitted instruction sequence.

Completion check:

- Focused coverage fails before the repair or directly guards the missing
  pointer-local address publication fact.

### Step 3: Repair Pointer-Local Address Publication

Goal: publish current address-valued assignments to scalar pointer locals
before dereference consumers.

Primary target: the semantic/prepared/MIR/AArch64 helper localized in Step 1.

Actions:

- Implement the smallest semantic repair at the owning publication boundary.
- Preserve prior pointer-derived address/lvalue and address-valued
  call-argument behavior.
- Avoid broad rewrites of unrelated external-call return, aggregate,
  floating-point, bit-field, timeout, or runner owners.
- Run build proof before focused and representative tests.

Completion check:

- Focused coverage passes and generated AArch64 no longer dereferences the
  stale `&a` pointer after `b = &(bolshevic.b)`.

### Step 4: Prove Representative And Classify Residual

Goal: prove `00163` advances past the stale pointer-home failure and classify
any next first bad fact.

Primary target: focused backend coverage and
`c_testsuite_aarch64_backend_src_00163_c`.

Actions:

- Run the supervisor-delegated build and proof command.
- Inspect generated `00163.c.s` enough to confirm the final `*b` consumes the
  global member address.
- If `00163` still fails, classify the new first bad fact and decide whether
  it remains in this idea or needs lifecycle handoff.
- Ask the supervisor whether backend-regex or broader regression guard proof
  is needed before closure.

Completion check:

- `00163` no longer fails because `b = &(bolshevic.b)` leaves `*b` consuming
  the old `&a` pointer home, and proof is recorded in `todo.md`.
