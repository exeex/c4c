# AArch64 Byval Aggregate Call Argument Lane Publication Runbook

Status: Active
Source Idea: ideas/open/328_aarch64_byval_aggregate_call_argument_lane_publication.md
Activated from: idea 327 Step 2 lifecycle split after commit de571342a

## Purpose

Repair the AArch64 caller-side handoff for small byval aggregate call
arguments whose prepared bytes are not published into the ABI argument
register lanes before a call.

## Goal

Ensure register-passed byval aggregate call arguments are loaded from their
prepared aggregate storage and packed into the required AAPCS64 integer
argument registers before `bl`.

## Core Rule

Progress must be a general AArch64 byval aggregate call-argument publication
capability. Do not special-case `00204.c`, `arg`, `fa_s1`, `fa_s2`, `s1`,
`s2`, `s17`, `[sp, #928]`, `x0`, one aggregate size, one source slot, or one
emitted call sequence. Do not reopen fixed-formal entry publication, HFA
variadic lowering, frame/formal publication, local/value-home publication,
global initializer emission, runner, expectation, unsupported, or proof-log
behavior unless fresh generated-code evidence proves that surface owns the
current first bad fact.

## Read First

- `ideas/open/328_aarch64_byval_aggregate_call_argument_lane_publication.md`
- `ideas/open/327_aarch64_fixed_formal_entry_publication.md`
- `ideas/open/326_aarch64_variadic_hfa_floating_residual.md`
- `todo.md`
- generated AArch64 artifacts for `00204.c` under
  `build/c_testsuite_aarch64_backend/src/`
- existing focused `test_after.log`, when present

## Current Targets

- Representative external case:
  - `c_testsuite_aarch64_backend_src_00204_c`
- Current first bad fact:
  - in caller function `arg`, callsites for `fa_s1` and `fa_s2` prepare local
    aggregate bytes in stack/frame storage
  - before `bl fa_s1`, the generated callsite does not publish the prepared
    one-byte aggregate into `x0`
  - before `bl fa_s2`, the generated callsite does not pack the prepared
    two-byte aggregate into the low lanes of `x0`
  - callee-side entry publication from idea 327 now expects and publishes
    incoming ABI register lanes correctly
- Prior-owner guardrails to preserve:
  - fixed-formal entry publication coverage from idea 327
  - local/value-home publication coverage from idea 325
  - frame/formal coverage from idea 324
  - HFA global initializer, fixed HFA argument, and fixed HFA return coverage
    from idea 326 execution

## Non-Goals

- Do not repair HFA/floating `va_arg` source selection, register-save-area
  progression, overflow-area progression, or HFA lane materialization unless
  the representative gets past byval aggregate call-argument publication and
  fresh evidence reaches those paths.
- Do not reopen callee fixed-formal entry publication unless generated code
  again shows an unpublished fixed formal being consumed before first use.
- Do not change semantic admission, runners, timeout policy, expectations,
  unsupported classifications, CTest registration, or proof-log policy.
- Do not claim completion from classification alone; generated callsites must
  publish byval aggregate bytes into ABI register lanes before branch.

## Working Model

Prepared AArch64 lowering can materialize a byval aggregate call argument in a
local object or frame slot, while AAPCS64 classifies small aggregates for
integer register passing. The call-lowering path must bridge that choice by
loading the prepared bytes, packing them into the low-to-high lanes of the
assigned integer argument register or registers, and emitting those register
values before the branch. Size 1 through 16 byval aggregates classified for
register transport need lane publication; larger byval aggregates that AAPCS64
stack-passes must remain stack-passed.

## Execution Rules

- Keep routine packet progress in `todo.md`.
- Localize the semantic BIR call argument, prepared source storage, ABI
  classification, register-lane destination, and emitted callsite consumer
  before editing code.
- Prefer focused backend coverage for byval aggregate call-argument
  publication before relying on the external c-testsuite representative.
- Preserve prior-owner guardrails; do not weaken fixed-formal entry
  publication, local/value-home publication, frame/formal publication,
  HFA argument/return, global data emission, runner, expectation, or proof-log
  coverage.
- If `00204.c` advances past `fa_s1` and `fa_s2`, record the next first bad
  fact in `todo.md` and return it to lifecycle classification if it belongs
  to another owner.
- Use narrow build plus focused CTest proof for implementation packets.
  Escalate to broader backend validation only when the supervisor requests it
  or the implementation touches shared backend helper/printer behavior
  broadly.

## Ordered Steps

### Step 1: Localize Byval Aggregate Call-Argument Lane Gap

Goal: identify the exact AArch64 call-argument records and emitted callsites
that leave register-passed byval aggregate bytes unpublished before `bl`.

Primary target: prepared AArch64 call-lowering state and generated `00204.c`
artifacts for caller `arg`.

Actions:

- Trace `s1` and `s2` from semantic BIR call arguments through prepared
  storage into generated AArch64 callsites for `fa_s1` and `fa_s2`.
- Map the prepared source bytes or frame slots, ABI classification, destination
  register lanes, and the emitted branch point.
- Identify whether the missing bridge belongs to prepared call-argument
  assignment, aggregate source loading, register-lane packing, or instruction
  emission.
- Record in `todo.md` the owning code surfaces, representative tests, and the
  smallest focused proof command for the repair.

Completion check:

- `todo.md` names the byval arguments, prepared source storage, ABI register
  lanes, owning code surfaces, representative tests, and smallest focused
  proof command.

### Step 2: Repair General Byval Aggregate Register-Lane Publication

Goal: publish register-passed byval aggregate call arguments from prepared
storage into AAPCS64 integer argument register lanes before calls.

Primary target: AArch64 prepared/codegen call-argument lowering.

Actions:

- Implement a general bridge from each register-passed byval aggregate source
  object to its assigned ABI integer argument register lane or lanes.
- Pack source bytes in the correct order for one-register and multi-register
  small aggregates, including sizes 1 through 16 when classified for register
  transport.
- Preserve scalar argument handling, pointer byval metadata, variadic call
  setup, and stack-passed large aggregate behavior.
- Reuse existing AArch64 register, stack-slot, source-load, address
  materialization, and move/store helpers when available.

Completion check:

- Focused proof shows small byval aggregate call arguments are published from
  prepared storage to ABI register lanes before branch, without regressing
  adjacent publication guardrails.

### Step 3: Add Focused Call-Argument Lane Coverage

Goal: make the caller-side byval aggregate ABI handoff observable in local
backend tests.

Primary target: existing AArch64 prepared-BIR, machine-printer, instruction
dispatch, call-lowering, or focused `00204.c` dump tests.

Actions:

- Add or extend focused coverage for a byval aggregate call argument whose
  prepared source lives in frame/object storage but whose ABI assignment uses
  integer register lanes.
- Cover at least one single-register small aggregate and one multi-byte or
  multi-lane aggregate shape.
- Assert the generated or prepared output includes publication into ABI
  argument registers before `bl`.
- Preserve adjacent scalar, stack-passed aggregate, fixed-formal entry,
  local/value-home, and frame/formal coverage.

Completion check:

- Local coverage fails without the call-argument publication repair and passes
  with the repair.

### Step 4: Validate Representative And Classify Residuals

Goal: prove the caller-side byval aggregate publication repair on the focused
representative and classify any newly exposed first bad fact.

Primary target: supervisor-selected focused proof scope including `00204.c`.

Actions:

- Run a focused proof including build, local AArch64 backend coverage,
  prior-owner guardrails, and `c_testsuite_aarch64_backend_src_00204_c`.
- Confirm generated `arg` publishes byval aggregate bytes into ABI register
  lanes before calls such as `fa_s1` and `fa_s2`.
- If `00204.c` still fails, classify whether the next first bad fact belongs
  to idea 326's HFA/floating residual path or another distinct initiative.
- Record pass/fail results and first bad facts in `todo.md`.

Completion check:

- `todo.md` records fresh proof. The byval aggregate call-argument
  register-lane publication fault is gone, and any remaining blocker is
  explicitly localized for the next lifecycle decision.
