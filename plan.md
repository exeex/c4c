# AArch64 Fixed Formal Entry Publication Runbook

Status: Active
Source Idea: ideas/open/327_aarch64_fixed_formal_entry_publication.md
Activated from: idea 326 Step 4 adjacent-owner classification

## Purpose

Repair the AArch64 entry-time handoff for fixed function parameters whose
prepared homes differ from their AAPCS64 incoming argument registers.

## Goal

Ensure each fixed formal is published from its ABI incoming location into its
assigned prepared home before generated code can load, store, or otherwise
consume that formal.

## Core Rule

Progress must be a general AArch64 fixed-formal entry publication capability.
Do not special-case `00204.c`, `myprintf`, `%p.format`, `x0`, `x13`, one
register, one stack slot, one pointer type, or one emitted prologue sequence.
Do not reopen HFA/floating variadic consume, global initializer, fixed HFA
argument, fixed HFA return, local/value-home, frame-layout, runner,
expectation, unsupported, or proof-log behavior unless fresh generated-code
evidence proves that surface owns the current first bad fact.

## Read First

- `ideas/open/327_aarch64_fixed_formal_entry_publication.md`
- `ideas/open/326_aarch64_variadic_hfa_floating_residual.md`
- `ideas/closed/325_aarch64_variadic_local_value_home_publication.md`
- `ideas/closed/324_aarch64_variadic_frame_formal_publication.md`
- `todo.md`
- generated AArch64 artifacts for `00204.c` under
  `build/c_testsuite_aarch64_backend/src/`
- existing focused `test_before.log` / `test_after.log`, when present

## Current Targets

- Representative external case:
  - `c_testsuite_aarch64_backend_src_00204_c`
- Current first bad fact:
  - `stdarg` calls `myprintf("%9s ...", ...)` with the fixed `format`
    pointer in AAPCS64 incoming `x0`
  - prepared AArch64 storage assigns `%p.format` to `register:x13`
  - generated `myprintf` stores or consumes the `x13`-derived value before any
    `x0 -> x13` or `x0 -> [home]` publication
  - runtime then reaches `ldrb w9, [x10]` with `x10 = 0`
- Prior-owner guardrails to preserve:
  - semantic `myprintf(ptr %p.format, ...)` BIR handoff
  - prepared-BIR focus and AArch64 publication dumps for `00204.c`
  - local/value-home publication coverage from idea 325
  - frame/formal coverage from idea 324
  - HFA global initializer, fixed HFA argument, and fixed HFA return coverage
    from idea 326 execution

## Non-Goals

- Do not repair HFA/floating `va_arg` source selection, register-save-area
  progression, overflow-area progression, or HFA lane materialization unless
  the representative gets past fixed-formal publication and fresh evidence
  reaches those paths.
- Do not reopen global initializer emission, fixed HFA argument lanes, fixed
  HFA return lanes, local/value-home publication, `va_start` destination
  publication, aggregate helper text lowering, F128 transport, scalar ALU
  immediate materialization, large frame adjustment, or stack-slot spelling
  without direct generated-code evidence.
- Do not change semantic admission, runners, timeout policy, expectations,
  unsupported classifications, CTest registration, or proof-log policy.
- Do not claim completion from classification alone; generated code must
  publish fixed formals before first use.

## Working Model

Prepared AArch64 lowering can assign a BIR fixed parameter to a home that is
not its ABI incoming argument register. Function-entry code must bridge that
choice by copying or storing from the AAPCS64 incoming location into the
prepared home before local stores, cursor setup, or first-use loads consume the
formal. The current representative exposes this with `%p.format`: incoming
`x0` is valid at entry, but assigned `x13` is never initialized before the
format loop dereferences the resulting cursor.

## Execution Rules

- Keep routine packet progress in `todo.md`.
- Localize the parameter's semantic BIR formal, prepared storage record, ABI
  incoming location, assigned home, and first generated consumer before editing
  code.
- Prefer focused backend coverage for entry publication before relying on the
  external c-testsuite representative.
- Preserve prior-owner guardrails; do not weaken prepared handoff,
  frame/formal publication, local/value-home publication, HFA argument/return,
  global data emission, runner, expectation, or proof-log coverage.
- If `00204.c` advances past the null dereference, record the next first bad
  fact in `todo.md` and return it to lifecycle classification if it belongs to
  another owner.
- Use narrow build plus focused CTest proof for implementation packets.
  Escalate to broader backend validation only when the supervisor requests it
  or the implementation touches shared backend helper/printer behavior broadly.

## Ordered Steps

### Step 1: Localize Fixed Formal Entry Publication Gap

Goal: identify the exact AArch64 parameter handoff records and emitted code
that leave a fixed formal unpublished before first use.

Primary target: prepared AArch64 storage, function-entry codegen, and
generated `00204.c` artifacts for `myprintf`.

Actions:

- Trace `%p.format` from semantic BIR formal through prepared storage and into
  generated AArch64 prologue or first-use code.
- Map the ABI incoming location (`x0` for the first fixed pointer formal) and
  assigned prepared home (`register:x13` in the current evidence).
- Identify whether the missing bridge belongs to prepared value-home/regalloc
  handling, function-entry parameter publication, or instruction emission.
- Record in `todo.md` the owning code surfaces, representative tests, and the
  smallest focused proof command for the repair.

Completion check:

- `todo.md` names the fixed formal, ABI incoming location, assigned home,
  first consumer, owning code surfaces, representative tests, and smallest
  focused proof command.

### Step 2: Repair General Fixed Formal Entry Publication

Goal: publish fixed parameters from AAPCS64 incoming locations into assigned
prepared homes before first use.

Primary target: AArch64 prepared/codegen function-entry parameter publication.

Actions:

- Implement a general bridge from each fixed formal's ABI incoming location to
  its assigned prepared home when those locations differ.
- Cover register homes and stack or memory homes as the existing prepared-home
  model requires.
- Preserve variadic register-save-area setup and local/value-home publication
  behavior; the entry publication must not clobber saved variadic state.
- Reuse existing AArch64 register, stack-slot, prepared-home, address
  materialization, and move/store helpers when available.

Completion check:

- Focused proof shows ordinary fixed formals are published from ABI incoming
  locations to assigned homes before first use, without regressing adjacent
  publication guardrails.

### Step 3: Add Focused Entry Publication Coverage

Goal: make the repaired fixed-formal entry contract observable in local
backend tests.

Primary target: existing AArch64 prepared-BIR, machine-printer, instruction
dispatch, function prologue, or focused `00204.c` dump tests.

Actions:

- Add or extend focused coverage for a variadic callee with an ordinary fixed
  pointer formal assigned to non-ABI storage.
- Assert the generated or prepared output includes publication from the ABI
  incoming location into the assigned home before first use.
- Preserve adjacent scalar, aggregate, variadic register-save-area,
  local/value-home, and frame/formal coverage.

Completion check:

- Local coverage fails without the entry-publication repair and passes with
  the repair.

### Step 4: Validate Representative And Classify Residuals

Goal: prove the fixed-formal entry repair on the focused representative and
classify any newly exposed first bad fact.

Primary target: supervisor-selected focused proof scope including `00204.c`.

Actions:

- Run a focused proof including build, local AArch64 backend coverage,
  prior-owner guardrails, and `c_testsuite_aarch64_backend_src_00204_c`.
- Confirm generated `myprintf` no longer consumes an unpublished `%p.format`
  value before first use.
- If `00204.c` still fails, classify whether the next first bad fact belongs
  to idea 326's HFA/floating residual path or another distinct initiative.
- Record pass/fail results and first bad facts in `todo.md`.

Completion check:

- `todo.md` records fresh proof. The `%p.format` entry-publication fault is
  gone, and any remaining blocker is explicitly localized for the next
  lifecycle decision.
