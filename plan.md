# AArch64 Variadic HFA And Floating Residual Runbook

Status: Active
Source Idea: ideas/open/326_aarch64_variadic_hfa_floating_residual.md
Activated from: idea 330 closure handoff

## Purpose

Resume the parked AArch64 variadic HFA/floating residual now that intervening
fixed-formal, call-publication, byval aggregate, and non-HFA aggregate
`va_arg` blockers have been repaired.

## Goal

Classify and repair the current `00204.c` HFA/floating or long-double runtime
blocker generally for AArch64 variadic generated code.

## Core Rule

Progress must repair a concrete AArch64 variadic HFA/floating owner. Do not
special-case `00204.c`, `myprintf`, one HFA shape, one float literal, one
long-double branch, one stack offset, one temporary slot, one format string,
or one emitted instruction sequence. Do not reopen the closed non-HFA `%7s` /
`%9s` aggregate string materialization route unless fresh generated-code
evidence shows that copied string aggregate bytes are again missing before
their observing calls.

## Read First

- `ideas/open/326_aarch64_variadic_hfa_floating_residual.md`
- `ideas/closed/330_aarch64_non_hfa_aggregate_va_arg_materialization.md`
- `todo.md`
- generated AArch64 artifacts for `00204.c` under
  `build/c_testsuite_aarch64_backend/src/`
- existing focused `test_after.log`, when present

## Current Targets

- Representative external case:
  - `c_testsuite_aarch64_backend_src_00204_c`
- Current first bad fact:
  - the representative gets past non-HFA string aggregate `va_arg`
    materialization for `%7s` and `%9s`
  - runtime output reaches the HFA/floating region, prints malformed numeric
    values, and eventually segfaults
  - generated `myprintf` HFA branches select aggregate `va_arg` payloads into
    temporary stack slots such as `sp + 720` / `sp + 724`, but following
    float/double publications read different homes such as `sp + 556` /
    `sp + 560`
  - long-double HFA branches select stack payload slots and then call
    `printf` with `.str44` without publishing the selected long-double values
    as `printf` arguments
- Prior-owner guardrails to preserve:
  - non-HFA aggregate `va_arg` source-to-destination copies from idea 330
  - post-`va_arg` call operand publication from idea 329
  - byval aggregate call-argument lane publication from idea 328
  - fixed-formal entry publication from idea 327
  - HFA global initializer, fixed HFA argument, and fixed HFA return coverage
    from earlier idea 326 execution
  - local/value-home publication from idea 325
  - frame/formal coverage from idea 324

## Non-Goals

- Do not repair non-HFA aggregate string `va_arg` materialization unless fresh
  evidence shows selected `%7s` / `%9s` bytes are no longer copied before the
  following `printf`.
- Do not reopen local/value-home publication, fixed-formal entry publication,
  post-`va_arg` ordinary-call publication, byval aggregate call lanes,
  frame/formal publication, global initializer emission, runner behavior,
  expectation files, unsupported classifications, timeout policy, CTest
  registration, or proof-log policy without direct generated-code evidence.
- Do not claim progress from classification alone; generated code must publish
  the selected HFA/floating or long-double payload through the correct ABI
  argument homes before the following call observes it.

## Working Model

The current representative failure is downstream of non-HFA aggregate string
materialization. Selected HFA/floating payloads appear to be materialized into
one set of temporary homes while call-boundary publication reads another, and
long-double branches appear to reach `printf` without publishing selected
values. The next repair should determine whether the owner is HFA lane
materialization, floating register-save-area or overflow-area progression,
aggregate/floating `va_arg` source selection, call-boundary argument
transport, or a narrower generated-code publication break with direct
evidence.

## Execution Rules

- Keep routine packet progress in `todo.md`.
- Localize the first HFA/floating or long-double bad branch before editing
  code.
- Prefer focused backend coverage for the repaired HFA/floating owner before
  relying on the external c-testsuite representative.
- Preserve prior-owner guardrails; do not weaken non-HFA aggregate
  materialization, call publication, byval aggregate, fixed-formal,
  local/value-home, frame/formal, or HFA fixed argument/return coverage.
- If `00204.c` advances past the current HFA/floating residual, record the
  next first bad fact in `todo.md` and return it to lifecycle classification
  if it belongs to another owner.

## Ordered Steps

### Step 1: Localize Current HFA/Floating Residual

Goal: identify the first branch where selected HFA/floating or long-double
`va_arg` payloads diverge from the values published to the following call.

Primary target: generated `myprintf` artifacts and AArch64 `va_arg`
materialization/publication state for the first malformed HFA/floating output
after the repaired `%7s` / `%9s` branches.

Actions:

- Trace the first malformed HFA/floating output from source expectation to
  semantic value, prepared payload, selected `va_arg` source, temporary home,
  final ABI argument publication, and observing `printf` call.
- Compare selected payload homes such as `sp + 720` / `sp + 724` against
  published homes such as `sp + 556` / `sp + 560` and determine which owner
  makes them diverge.
- Inspect the long-double HFA branch that reaches `.str44` without publishing
  selected values as `printf` arguments.
- Classify whether the owner is source selection/progression, HFA lane
  materialization, temporary-object publication, call-boundary transport, or a
  distinct generated-code path.
- Record in `todo.md` the first bad branch, selected source, expected
  payload, observed publication, owning code surfaces, representative tests,
  and smallest focused proof command.

Completion check:

- `todo.md` names the concrete first HFA/floating or long-double bad fact and
  the code owner to repair, without reopening closed non-HFA aggregate string
  materialization.

### Step 2: Repair The Classified HFA/Floating Owner

Goal: make generated AArch64 publish the selected HFA/floating or long-double
payload through the correct ABI argument homes before the following call.

Primary target: the generated-code owner localized by Step 1.

Actions:

- Repair the classified owner generally for AArch64 variadic HFA/floating
  behavior.
- Preserve source address, destination home, lane order, byte count, register
  class, and call ordering for the selected payload.
- Include both register-save-area and overflow-area paths when Step 1 shows
  the owner spans both.
- Scope the change to the classified owner; do not add named-case emission
  shortcuts or broad call-boundary replay.
- Preserve scalar `va_arg`, non-HFA aggregate `va_arg`, ordinary calls, fixed
  aggregate calls, byval aggregate lane publication, and stack-passed large
  aggregate behavior.

Completion check:

- Focused proof shows the selected HFA/floating or long-double payload is
  published to the following call from the correct homes, with adjacent
  guardrails still stable.

### Step 3: Add Focused HFA/Floating Variadic Coverage

Goal: make the repaired HFA/floating owner observable in local backend tests.

Primary target: existing AArch64 semantic/prepared BIR dump tests, machine
printer tests, instruction dispatch tests, or focused `00204.c` dump tests.

Actions:

- Add or extend focused coverage for the repaired owner from Step 2.
- Assert selected payload source, temporary home, final ABI publication, and
  following call order.
- Cover the current representative shape or a smaller local equivalent that
  fails without the same owner repair.
- Preserve adjacent scalar `va_arg`, non-HFA aggregate `va_arg`, post-`va_arg`
  call publication, fixed aggregate call, byval aggregate lane,
  fixed-formal entry, local/value-home, and frame/formal coverage.

Completion check:

- Local coverage fails without the HFA/floating repair and passes with the
  repair.

### Step 4: Validate Representative And Classify Residuals

Goal: prove the HFA/floating repair on the focused representative and
classify any newly exposed first bad fact.

Primary target: supervisor-selected focused proof scope including `00204.c`.

Actions:

- Run a focused proof including build, local AArch64 backend coverage,
  prior-owner guardrails, and `c_testsuite_aarch64_backend_src_00204_c`.
- Confirm generated `myprintf` still copies selected non-HFA aggregate bytes
  into the `%7s` / `%9s` destination buffers before passing those buffer
  addresses to `printf`.
- Confirm the repaired HFA/floating or long-double payload is published to
  the observing call through the correct ABI argument homes.
- If `00204.c` still fails, classify whether the next first bad fact remains
  in idea 326 scope or belongs to another distinct initiative.
- Record pass/fail results and first bad facts in `todo.md`.

Completion check:

- `todo.md` records fresh proof. The classified HFA/floating fault is gone,
  prior-owner guardrails remain stable, and any remaining blocker is
  explicitly localized for the next lifecycle decision.
