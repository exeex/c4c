# AArch64 Variadic Aggregate Va Arg Post-Consumption Call Setup Runbook

Status: Active
Source Idea: ideas/open/329_aarch64_variadic_aggregate_va_arg_call_setup.md
Activated from: idea 328 Step 4 lifecycle handoff

## Purpose

Repair the AArch64 generated-code path where a variadic function consumes a
non-HFA aggregate with `va_arg` and then performs an ordinary call whose
fixed operands are not fully published into ABI argument registers.

## Goal

Ensure post-`va_arg` ordinary calls publish the fixed format string and
aggregate-derived call operands into the required AAPCS64 argument registers
before `bl`.

## Core Rule

Progress must be a general AArch64 post-`va_arg` call-setup capability. Do
not special-case `00204.c`, `myprintf`, `%7s`, `%9s`, `.str31`, `.str33`,
`x0 == 0x1`, one aggregate size, one stack offset, one local temporary, or one
emitted `printf` sequence. Do not reopen byval aggregate fixed-call
publication, fixed-formal entry publication, HFA/floating variadic lowering,
frame/formal publication, local/value-home publication, global initializer
emission, runner, expectation, unsupported, or proof-log behavior unless
fresh generated-code evidence proves that surface owns the current first bad
fact.

## Read First

- `ideas/open/329_aarch64_variadic_aggregate_va_arg_call_setup.md`
- `ideas/open/328_aarch64_byval_aggregate_call_argument_lane_publication.md`
- `ideas/open/326_aarch64_variadic_hfa_floating_residual.md`
- `todo.md`
- generated AArch64 artifacts for `00204.c` under
  `build/c_testsuite_aarch64_backend/src/`
- existing focused `test_after.log`, when present

## Current Targets

- Representative external case:
  - `c_testsuite_aarch64_backend_src_00204_c`
- Current first bad fact:
  - generated `arg` now publishes byval aggregate `s1` and `s2` call
    arguments into ABI register lanes before `fa_s1` and `fa_s2`
  - the remaining crash is inside `myprintf`, after a non-HFA aggregate
    `va_arg` path
  - the `%9s` branch reaches libc `printf` with `x0 == 0x1` instead of
    loading `.str33` (`"%.9s"`) into `x0`
  - the `%7s` branch has the same missing `.str31` format-argument shape
  - `x1` is set to the aggregate text-buffer address, so the first observed
    missing operand is the fixed format string publication for the following
    call
- Prior-owner guardrails to preserve:
  - byval aggregate call-argument lane publication from idea 328
  - fixed-formal entry publication from idea 327
  - HFA global initializer, fixed HFA argument, and fixed HFA return coverage
    from idea 326 execution
  - local/value-home publication from idea 325
  - frame/formal coverage from idea 324

## Non-Goals

- Do not repair HFA/floating `va_arg` source selection, floating
  register-save-area progression, overflow-area progression, or HFA lane
  materialization unless the representative gets past this non-HFA aggregate
  post-`va_arg` call setup and fresh evidence reaches those paths.
- Do not reopen byval aggregate fixed-call publication unless generated code
  again shows fixed aggregate callsites such as `fa_s1` or `fa_s2` failing to
  publish prepared aggregate bytes into ABI register lanes before `bl`.
- Do not change semantic admission, runners, timeout policy, expectations,
  unsupported classifications, CTest registration, or proof-log policy.
- Do not claim completion from classification alone; generated callsites must
  publish the fixed format operand and aggregate-derived operand into ABI
  registers before branch.

## Working Model

The failing branch is an ordinary call that occurs after variadic aggregate
consumption. AArch64 lowering may have correct aggregate `va_arg` storage and
member address materialization, while still losing or skipping publication of
fixed call operands such as the format string constant. The repair should
bridge the post-`va_arg` call operand records to final ABI registers in the
same way as other ordinary calls: each operand must be materialized in call
order, survive any `va_arg` temporary setup, and be available in the assigned
argument register immediately before `bl`.

## Execution Rules

- Keep routine packet progress in `todo.md`.
- Localize semantic BIR, prepared BIR, aggregate `va_arg` state, call operand
  records, ABI register assignment, and generated machine instructions before
  editing code.
- Prefer focused backend coverage for the post-`va_arg` call setup before
  relying on the external c-testsuite representative.
- Preserve prior-owner guardrails; do not weaken byval aggregate call-lane
  publication, fixed-formal entry publication, local/value-home publication,
  frame/formal publication, HFA argument/return, global data emission, runner,
  expectation, or proof-log coverage.
- If `00204.c` advances past `%7s` and `%9s`, record the next first bad fact
  in `todo.md` and return it to lifecycle classification if it belongs to
  another owner.
- Use narrow build plus focused CTest proof for implementation packets.
  Escalate to broader backend validation only when the supervisor requests it
  or the implementation touches shared backend helper/printer behavior
  broadly.

## Ordered Steps

### Step 1: Localize Variadic Aggregate Va Arg Post-Consumption Call Setup

Goal: identify the exact generated-code owner that lets the post-`va_arg`
`printf` call branch without publishing the fixed format operand into `x0`.

Primary target: generated `myprintf` artifacts and AArch64 call-lowering state
for the `%7s` and `%9s` branches in `00204.c`.

Actions:

- Trace each failing branch from `match(&s, "%7s")` and `match(&s, "%9s")`
  through aggregate `va_arg` consumption to the following `printf` call.
- Map the semantic call operands, prepared call operands, aggregate temporary
  storage, aggregate-member address, string literal source, ABI argument
  registers, and emitted branch point.
- Determine whether `.str31`/`.str33` is missing because of call-operand
  materialization, string-constant lowering, argument register assignment,
  instruction ordering, `va_arg` temporary lifetime, or a clobber before
  `bl printf`.
- Record in `todo.md` the owning code surfaces, representative tests, and the
  smallest focused proof command for the repair.

Completion check:

- `todo.md` names the failing branches, call operands, aggregate `va_arg`
  state, expected ABI registers, observed emitted instructions, owning code
  surfaces, representative tests, and smallest focused proof command.

### Step 2: Repair General Post-Va Arg Ordinary Call Operand Publication

Goal: publish ordinary call operands after aggregate `va_arg` consumption into
their assigned AAPCS64 argument registers before the call.

Primary target: AArch64 prepared/codegen call-argument lowering for ordinary
calls following variadic aggregate consumption.

Actions:

- Implement a general bridge from post-`va_arg` call operand records to final
  ABI argument registers, including fixed string constants and
  aggregate-member pointer operands.
- Preserve operand ordering so the fixed format string is assigned to `x0`
  and subsequent aggregate-derived operands are assigned to later ABI
  argument registers.
- Prevent aggregate `va_arg` temporary setup or member-address materialization
  from clobbering the final call argument registers before `bl`.
- Reuse existing AArch64 register, stack-slot, constant-address,
  aggregate-member, and move helpers when available.
- Preserve scalar calls, fixed aggregate calls, byval aggregate lane
  publication, variadic setup, and stack-passed large aggregate behavior.

Completion check:

- Focused proof shows post-`va_arg` ordinary calls publish fixed constants and
  aggregate-derived operands into ABI registers before branch without
  regressing adjacent publication guardrails.

### Step 3: Add Focused Post-Va Arg Call-Setup Coverage

Goal: make the failing non-HFA aggregate `va_arg` plus following ordinary call
observable in local backend tests.

Primary target: existing AArch64 semantic/prepared BIR dump tests, machine
printer tests, instruction dispatch tests, or focused `00204.c` dump tests.

Actions:

- Add or extend focused coverage for a variadic aggregate `va_arg` path
  followed by an ordinary call with a fixed string constant in `x0` and an
  aggregate-member pointer or equivalent value in a later ABI argument
  register.
- Assert the generated or prepared output includes publication of the fixed
  format string and aggregate-derived operand before `bl`.
- Cover the `%7s`/`%9s` shape or a smaller local equivalent that fails
  without the same owner repair.
- Preserve adjacent scalar call, fixed aggregate call, byval aggregate lane,
  fixed-formal entry, local/value-home, frame/formal, and HFA/floating
  coverage.

Completion check:

- Local coverage fails without the post-`va_arg` call-setup repair and passes
  with the repair.

### Step 4: Validate Representative And Classify Residuals

Goal: prove the post-`va_arg` call-setup repair on the focused representative
and classify any newly exposed first bad fact.

Primary target: supervisor-selected focused proof scope including `00204.c`.

Actions:

- Run a focused proof including build, local AArch64 backend coverage,
  prior-owner guardrails, and `c_testsuite_aarch64_backend_src_00204_c`.
- Confirm generated `myprintf` loads `.str31`/`.str33` or equivalent format
  string constants into `x0` and publishes aggregate text-buffer operands into
  the expected following argument registers before `bl printf`.
- If `00204.c` still fails, classify whether the next first bad fact belongs
  to idea 326's HFA/floating residual path or another distinct initiative.
- Record pass/fail results and first bad facts in `todo.md`.

Completion check:

- `todo.md` records fresh proof. The post-`va_arg` ordinary call-setup fault
  is gone, and any remaining blocker is explicitly localized for the next
  lifecycle decision.
