# AArch64 Calls Prepared Authority Repair Runbook

Status: Active
Source Idea: ideas/open/52_aarch64_calls_prepared_authority_repair.md
Supersedes active route: ideas/open/51_aarch64_alu_prepared_authority_repair.md is parked open after Step 7 classified the remaining focused failure as calls/variadic ownership, not ordinary ALU stale-home ownership.

## Purpose

Turn AArch64 call, call-boundary, and variadic helper lowering into consumers
of prepared call plans, argument/result plans, source selections, boundary
effects, move bundles, value homes, and variadic access plans instead of
rediscovering ABI source and cursor authority locally.

## Goal

Repair the remaining `c_testsuite_aarch64_backend_src_00204_c` failure first by
making small variadic aggregate `va_arg` lowering use the prepared AAPCS64
register-save and overflow cursor authority for byval aggregates such as
`struct s9`.

## Core Rule

Prefer shared prepared call, variadic access-plan, source-selection, and
boundary-effect facts over raw call operands, callee-name analysis, BIR value
spelling, frame-slot naming, recursive same-block producer walks, or
named-case workarounds.

## Read First

- `ideas/open/52_aarch64_calls_prepared_authority_repair.md`
- `todo.md`
- `src/backend/mir/aarch64/codegen/calls.cpp`
- `src/backend/mir/aarch64/codegen/variadic.cpp`
- `src/backend/mir/aarch64/codegen/variadic.hpp`
- `src/backend/mir/aarch64/codegen/instruction.hpp`
- `src/backend/prealloc/prepared_lookups.cpp`
- prepared call and variadic facts for:
  - `PreparedCallPlan`
  - `PreparedCallArgumentPlan`
  - `PreparedCallArgumentSourceSelection`
  - `PreparedCallBoundaryEffectPlan`
  - `PreparedCallPreservedValue`
  - `PreparedVariadicEntryPlanFunction`
  - `PreparedVariadicAggregateVaArgAccessPlan`

## Current Targets

- `complete_variadic_call_record`
- `make_variadic_aggregate_va_arg_record`
- `print_aggregate_va_arg_lowering_lines`
- `append_aggregate_copy_from_va_list_field`
- aggregate va_arg overflow cursor advancement
- aggregate va_arg register-save versus overflow source selection
- byval aggregate call argument register/stack placement for the `00204`
  `struct s9` case
- remaining `calls.cpp` duplicated authority listed in the source idea after
  the variadic failure is reduced

## Non-Goals

- Do not change AAPCS64 register/stack staging, `bl`/`blr` spelling, stack
  cleanup, source reload sequencing, ABI result store spelling, or variadic
  layout constants except to consume or correct prepared facts.
- Do not implement a broad call lowering rewrite.
- Do not push call-argument producer logic into ALU or dispatch value
  materialization follow-ups without an explicit shared-query contract.
- Do not downgrade expectations, mark supported tests unsupported, or claim
  helper renames as capability progress.
- Do not close the parked ALU idea as part of this route switch.

## Working Model

- The caller-side prepared plan for the first `stdarg` call in `00204` appears
  coherent: the format pointer is in `x0`, the first three `struct s9`
  variadic byval arguments are passed in register pairs `x1/x2`, `x3/x4`, and
  `x5/x6`, and the remaining three are passed in outgoing stack slots.
- The callee-side `myprintf` prologue saves general-purpose argument registers
  into the register-save area and initializes a negative `gp_offset`.
- The observed crash is in `%9s` aggregate `va_arg` handling when the overflow
  path dereferences `x13 = 0x10`; the suspected authority gap is cursor
  advancement from a spill slot instead of the loaded overflow pointer.
- Variadic helper machine nodes are selected through `calls.cpp`, completed
  from prepared variadic facts, and printed/lowered through `variadic.cpp`.
  Keep edits aligned with that ownership instead of adding testcase-specific
  branches.

## Execution Rules

- Start from the focused proof that currently passes 5/6 and fails only
  `c_testsuite_aarch64_backend_src_00204_c`.
- Preserve `00164`, `00176`, `00181`, and the two ALU probes as guardrails
  while repairing `00204`.
- Add or use nearby semantic backend probes for variadic aggregate `va_arg`
  register-save and overflow paths before relying on one c-testsuite case.
- If tracing proves the concrete repair belongs wholly outside calls,
  call-boundary, or variadic prepared call authority, stop and request a source
  idea split instead of widening this plan silently.
- Each code-changing step needs fresh build or compile proof. The supervisor
  chooses the exact proving subset and any broader validation checkpoint.

## Steps

### Step 1: Establish variadic aggregate va_arg cursor baseline

Goal: make the remaining failure and the prepared variadic facts explicit
before changing lowering.

Primary target: `todo.md` proof state and focused dumps for `00204`

Actions:

- Re-run or reuse the supervisor-selected focused subset that currently passes
  5/6 and fails only `c_testsuite_aarch64_backend_src_00204_c`.
- Dump BIR, prepared BIR, and relevant machine/assembly for `myprintf` and the
  first `stdarg` call.
- Record the aggregate access plan fields for the first `%9s` `va_arg`:
  source class, progression field, overflow source field, register-save lane
  homes, strides, source va_list home, and cursor update sequence.
- Confirm whether the bad `x13 = 0x10` value comes from prepared fact
  selection, record construction, or printer/lowering emission.

Completion check:

- `todo.md` records the exact cursor source, the prepared fact that should own
  it, and the first bounded code packet.

### Step 2: Add focused variadic aggregate probes

Goal: prove the repair against same-feature variadic aggregate paths, not only
against c-testsuite `00204`.

Primary target: `tests/backend/case/`

Actions:

- Add a minimal probe where a small aggregate variadic argument is consumed
  from the register-save area.
- Add a nearby probe where the same aggregate shape is consumed from the
  overflow area after register-passed variadic slots are exhausted.
- Keep expected behavior semantic; do not encode c-testsuite symbol names,
  instruction labels, temporary registers, or exact spill-slot numbering.

Completion check:

- The probes distinguish register-save and overflow cursor behavior and fail
  or pass for the same reason as the traced prepared authority.

### Step 3: Repair aggregate va_arg cursor authority

Goal: make aggregate `va_arg` lowering advance from the authoritative loaded
cursor for both register-save and overflow paths.

Primary target: `make_variadic_aggregate_va_arg_record`,
`print_aggregate_va_arg_lowering_lines`, and the prepared aggregate access
plan consumed by those helpers

Actions:

- Inspect whether `overflow_source_field`,
  `overflow_source_field_offset_bytes`, `overflow_stride_bytes`,
  `progression_field`, and `progression_stride_bytes` encode the needed
  cursor source and update destination.
- If prepared facts are correct, change lowering to update the va_list field
  from the loaded cursor plus stride instead of from an unrelated spill slot.
- If prepared facts are incomplete, add or consume the smallest shared
  prepared variadic access-plan fact instead of adding a local fallback.
- Preserve HFA and scalar `va_arg` behavior while repairing the small byval
  aggregate path.

Completion check:

- Focused variadic probes and `c_testsuite_aarch64_backend_src_00204_c` no
  longer fault from a bad overflow cursor, with no expectation downgrades.

### Step 4: Verify byval aggregate call-boundary source selection

Goal: keep caller-side aggregate argument placement aligned with prepared
call argument/source-selection authority.

Primary target: `calls.cpp` byval aggregate argument materialization paths

Actions:

- Use the `00204` first `stdarg` call as a guardrail: first three `struct s9`
  values should remain register-passed in pairs and later values stack-passed.
- Inspect `PreparedCallArgumentSourceSelection` use for byval register lanes,
  frame-slot values, prior preservation, and outgoing stack slots.
- Replace any local source recovery that conflicts with the prepared call plan
  before claiming the variadic consumer repair complete.

Completion check:

- Caller-side byval aggregate placement remains prepared-plan driven and
  agrees with the callee-side variadic access model.

### Step 5: Continue calls prepared-authority cleanup

Goal: reduce the duplicate call authority listed in the source idea after the
variadic aggregate blocker is fixed.

Primary target: `src/backend/mir/aarch64/codegen/calls.cpp`

Actions:

- Route immediate ABI binding lookup through shared argument binding facts.
- Route frame-slot argument move lookup through shared move or argument-plan
  facts.
- Route scalar call-argument and indirect-callee materialization through
  shared producer/source-selection facts.
- Route call-boundary source materialization and after-call result publication
  through prepared boundary/result authority.

Completion check:

- Remaining call lowering paths fail closed on missing prepared facts and no
  longer use raw operand, BIR-name, frame-slot-name, or move-bundle scans as
  durable authority.

### Step 6: Close or split the calls route

Goal: decide whether the source idea is complete or whether a separate
initiative remains.

Primary target: `plan.md`, `todo.md`, and supervisor-selected validation logs

Actions:

- Run the supervisor-selected broader validation for the calls route.
- Request reviewer scrutiny if any slice claims progress through a narrow
  named case, expectation rewrite, or retained local scan.
- Close only if the source idea acceptance criteria are satisfied; otherwise
  leave a bounded follow-up packet or ask the plan owner to split a distinct
  initiative.

Completion check:

- The source idea is either closed with regression proof or remains open with a
  precise next packet that does not hide route drift.
