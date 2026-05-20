# AArch64 Variadic HFA And Floating Residual Runbook

Status: Active
Source Idea: ideas/open/326_aarch64_variadic_hfa_floating_residual.md
Reactivated after: ideas/open/331_aarch64_variadic_stdarg_cursor_format_residual.md

## Purpose

Continue from the idea 331 handoff after the AArch64 `va_start`
overflow-cursor residual was repaired. The representative now fails earlier in
the caller-side HFA argument block, not in the stdarg cursor/format path.

## Goal

Classify and repair the current `00204.c` HFA/floating argument publication
blocker generally for AArch64 generated code.

## Core Rule

Progress must repair a concrete AArch64 HFA/floating argument publication
owner. Do not special-case `00204.c`, `arg`, `fa_hfa11`, `hfa11`, one HFA
shape, one float value, one stack offset, one register, or one emitted
instruction sequence.

## Read First

- `ideas/open/326_aarch64_variadic_hfa_floating_residual.md`
- `todo.md`
- generated AArch64 artifacts for `00204.c` under
  `build/c_testsuite_aarch64_backend/src/`
- existing focused `test_after.log`, when present

## Current Targets

- Representative external case:
  - `c_testsuite_aarch64_backend_src_00204_c`
- Current first bad fact:
  - runtime mismatch is in the `Arguments:` section before `stdarg:`
  - `fa_hfa11(hfa11)` should print `11.1` but prints `0.0`
  - generated caller `arg` loads `hfa11`, stores it to `[sp,#784]`, then
    passes the stale `s8` path through `[sp,#788]`
  - generated callee `fa_hfa11` consumes `s0` normally, so the first bad fact
    is caller HFA float argument publication/loading
- Prior-owner guardrails to preserve:
  - AArch64 `va_start` overflow cursor initialization from idea 331
  - HFA/floating return publication from idea 326
  - sret `x8` transport and large byval indirect pointer transport
  - byval aggregate register-lane allocation and fragmented lane publication
  - non-HFA aggregate `va_arg` source-to-destination copies from idea 330
  - post-`va_arg` call operand publication from idea 329
  - fixed-formal entry publication from idea 327
  - local/value-home publication from idea 325
  - frame/formal coverage from idea 324

## Non-Goals

- Do not reopen stdarg cursor/format or `va_start` overflow cursor
  initialization unless fresh generated-code evidence again places the first
  bad fact there.
- Do not reopen HFA/floating return publication unless fresh generated-code
  evidence places the first bad fact in return-value transport.
- Do not reopen non-HFA aggregate string `va_arg` materialization for `%7s` or
  `%9s` unless selected aggregate bytes are again missing before their
  observing calls.
- Do not reopen byval aggregate lane allocation/publication, sret `x8`
  transport, large byval indirect pointer transport, F128 memory transport,
  fixed-formal entry publication, local/value-home publication, frame/formal
  publication, runner behavior, expectation files, unsupported
  classifications, timeout policy, CTest registration, or proof-log policy
  without direct generated-code evidence.

## Working Model

The remaining failure is before the repaired stdarg path. The callee consumes
the AAPCS64 float argument register normally, while the caller appears to load
or publish from the wrong HFA lane home. The next repair must localize whether
the stale `s8` path is caused by HFA aggregate lane materialization,
call-argument source selection, caller stack-home publication, final ABI
register publication, or another caller-side generated-code owner before
editing implementation.

## Execution Rules

- Keep routine packet progress in `todo.md`.
- Localize the first caller HFA/floating argument divergence before editing
  code.
- Prefer focused backend coverage for the repaired HFA/floating owner before
  relying on the external c-testsuite representative.
- Preserve prior-owner guardrails; do not weaken stdarg cursor, `va_start`,
  non-HFA aggregate `va_arg`, byval aggregate lanes, fixed-formal,
  local/value-home, or frame/formal coverage.
- If `00204.c` advances past the current HFA/floating residual, record the
  next first bad fact in `todo.md` and return it to lifecycle classification
  if it belongs to another owner.

## Ordered Steps

### Step 1: Localize HFA/Floating Caller Argument Publication Residual

Goal: identify the first caller-side state where generated code diverges while
publishing `hfa11` for `fa_hfa11(hfa11)`.

Primary target: generated `arg` and `fa_hfa11` artifacts and AArch64 HFA
aggregate lane materialization, source selection, stack home, and final ABI
argument publication state.

Actions:

- Trace `hfa11` from source initialization through generated caller storage,
  reload, final `s0` publication, and the observing `fa_hfa11` call.
- Compare the generated source home for the expected `11.1` lane with the
  stale `s8` / `[sp,#788]` publication path.
- Determine whether the owner is HFA lane materialization, prepared call
  operand source selection, caller stack-home publication, final ABI register
  publication, or another caller-side generated-code path.
- Record in `todo.md` the first bad state, expected value, observed value,
  owning code surfaces, representative tests, and smallest focused proof
  command.

Completion check:

- `todo.md` names the concrete first HFA/floating argument bad fact and the
  code owner to repair, without reopening stdarg cursor or prior aggregate
  materialization owners.

### Step 2: Repair The Classified HFA/Floating Owner

Goal: make generated AArch64 publish the correct HFA/floating argument lane to
the observing call.

Primary target: the generated-code owner localized by Step 1.

Actions:

- Repair the classified owner generally for AArch64 HFA/floating argument
  publication behavior.
- Preserve source address, destination home, lane order, register class,
  floating width, and final call ordering for the selected HFA argument lane.
- Scope the change to the classified owner; do not add named-case emission
  shortcuts or broad call-boundary replay.
- Preserve scalar `va_arg`, non-HFA aggregate `va_arg`, stdarg cursor,
  `va_start`, HFA/floating returns, byval aggregate lanes, fixed aggregate
  calls, and stack-passed large aggregate behavior.

Completion check:

- Focused proof shows the selected HFA/floating argument lane is published
  from the correct home, with adjacent guardrails still stable.

### Step 3: Add Focused HFA/Floating Coverage

Goal: make the repaired HFA/floating owner observable in local backend tests.

Primary target: existing AArch64 semantic/prepared BIR dump tests, machine
printer tests, instruction dispatch tests, or focused `00204.c` dump tests.

Actions:

- Add or extend focused coverage for the repaired owner from Step 2.
- Assert source lane, temporary home, final ABI publication, and following call
  order for the current representative shape or a smaller local equivalent.
- Preserve adjacent scalar `va_arg`, non-HFA aggregate `va_arg`, stdarg
  cursor, `va_start`, post-`va_arg` call publication, fixed aggregate call,
  byval aggregate lane, HFA/floating return, fixed-formal entry,
  local/value-home, and frame/formal coverage.

Completion check:

- Local coverage fails without the HFA/floating repair and passes with the
  repair.

### Step 4: Validate Representative And Classify Residuals

Goal: prove the HFA/floating repair on the focused representative and classify
any newly exposed first bad fact.

Primary target: supervisor-selected focused proof scope including `00204.c`.

Actions:

- Run a focused proof including build, local AArch64 backend coverage,
  prior-owner guardrails, and `c_testsuite_aarch64_backend_src_00204_c`.
- Confirm the `fa_hfa11(hfa11)` call receives and prints `11.1` before the
  observing call.
- Confirm prior stdarg cursor, `va_start`, HFA/floating return, non-HFA
  aggregate `va_arg`, byval lane, sret `x8`, fixed-formal, local/value-home,
  and frame/formal guardrails remain stable.
- If `00204.c` still fails, classify whether the next first bad fact remains
  in this source idea or belongs to another distinct initiative.
- Record pass/fail results and first bad facts in `todo.md`.

Completion check:

- `todo.md` records fresh proof. The classified HFA/floating argument fault is
  gone, prior-owner guardrails remain stable, and any remaining blocker is
  explicitly localized for the next lifecycle decision.
