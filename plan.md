# AArch64 Variadic Stdarg Field Residual Runbook

Status: Active
Source Idea: ideas/open/331_aarch64_variadic_stdarg_cursor_format_residual.md
Reactivated after: ideas/open/328_aarch64_byval_aggregate_call_argument_lane_publication.md

## Purpose

Continue from the idea 328 handoff after fixed non-HFA byval caller/callee
placement was repaired. The representative now fails later in the `stdarg:`
block, after the repaired byval `Arguments:` and `Return values:` sections
match.

## Goal

Classify and repair the next AArch64 variadic stdarg runtime blocker so the
`00204.c` stdarg block produces the expected sequence of fixed-width
aggregate string fields before advancing to any later residual.

## Core Rule

Progress must repair a general AArch64 variadic stdarg owner. Do not
special-case `00204.c`, `myprintf`, one `%9s` field, one `ABCDEFGHI` string,
one cursor value, one stack offset, one register, or one emitted instruction
sequence.

## Read First

- `ideas/open/331_aarch64_variadic_stdarg_cursor_format_residual.md`
- `todo.md`
- generated AArch64 artifacts for `00204.c` under
  `build/c_testsuite_aarch64_backend/src/`
- existing focused `test_after.log`, when present

## Current Targets

- Representative external case:
  - `c_testsuite_aarch64_backend_src_00204_c`
- Current first bad fact:
  - the byval `Arguments:` and `Return values:` sections now match through the
    repaired fixed aggregate case
  - the first visible mismatch is later in `stdarg:`
  - actual output begins `ABCDEFGHI ABCDEFGHI ABCDEFGHI stdarg:`
  - expected output has six `ABCDEFGHI` fields before `stdarg:`
  - this may be a stdarg field production, `va_list` cursor progression,
    register-save-area or overflow-area cursor, byval aggregate consumption
    order, or post-`va_arg` call-argument publication owner; localize before
    editing
- Prior-owner guardrails to preserve:
  - AArch64 `va_start` overflow cursor initialization from idea 331
  - HFA/floating argument and return publication from idea 326
  - fixed non-HFA byval rounded slot placement and outgoing stack argument
    base from idea 328
  - sret `x8` transport and large byval indirect pointer transport
  - non-HFA aggregate `va_arg` source-to-destination copies from idea 330
  - post-`va_arg` call operand publication from idea 329
  - fixed-formal entry publication from idea 327
  - local/value-home publication from idea 325
  - frame/formal coverage from idea 324

## Non-Goals

- Do not reopen HFA/floating argument or return publication unless fresh
  generated-code evidence again places the first bad fact there.
- Do not reopen fixed non-HFA byval register-lane allocation, rounded byval
  slot accounting, outgoing stack argument base, fragmented byval lane
  publication, or callee byval entry unpack unless generated-code evidence
  again places the first bad fact there.
- Do not reopen `va_start` overflow cursor initialization unless generated
  evidence shows the cursor base itself is again wrong.
- Do not reopen non-HFA aggregate string `va_arg` materialization for `%7s` or
  `%9s` unless selected aggregate bytes are again missing before their
  observing calls.
- Do not rewrite fixed-formal entry publication, local/value-home publication,
  frame/formal publication, runner behavior, expectation files, unsupported
  classifications, timeout policy, CTest registration, or proof-log policy
  without direct generated-code evidence.

## Working Model

The representative has advanced beyond the repaired fixed byval call-boundary
fault. The remaining mismatch drops three expected `ABCDEFGHI` fields before
the visible `stdarg:` marker. The owner must be proven from generated
artifacts: it could be stdarg format traversal, `va_list` cursor progression,
source aggregate selection, argument publication for the observing call, or a
nearby generated-code path.

## Execution Rules

- Keep routine packet progress in `todo.md`.
- Localize stdarg field production and observation before editing code.
- Prefer focused backend coverage for the repaired stdarg owner before relying
  on the external c-testsuite representative.
- Preserve prior-owner guardrails; do not weaken HFA/floating, stdarg cursor,
  `va_start`, non-HFA aggregate `va_arg`, fixed-formal, local/value-home, or
  frame/formal coverage.
- If `00204.c` advances past the current stdarg residual, record the next
  first bad fact in `todo.md` and return it to lifecycle classification if it
  belongs to another owner.

## Ordered Steps

### Step 1: Localize Later Stdarg Field Mismatch

Goal: identify the first generated-code state where the stdarg block loses
three expected `ABCDEFGHI` fields before the visible `stdarg:` marker.

Primary target: generated `myprintf` stdarg control flow, format traversal,
`va_list` cursor state, selected aggregate bytes, and the callsite that
observes each `ABCDEFGHI` field.

Actions:

- Trace the expected six `ABCDEFGHI` fields from format parsing through
  selected source bytes, cursor advancement, destination buffers, and emitted
  call operands.
- Determine whether the first loss is in stdarg format traversal, `va_list`
  cursor progression, register-save-area or overflow-area state, byval
  aggregate argument consumption, post-`va_arg` call publication, or another
  concrete generated-code path.
- Classify whether the owner is `src/backend/prealloc/call_plans.cpp`,
  `src/backend/prealloc/regalloc.cpp`,
  `src/backend/mir/aarch64/codegen/calls.cpp`,
  `src/backend/mir/aarch64/codegen/dispatch.cpp`, a stdarg lowering helper,
  or another narrower helper under the generated-code surfaces.
- Record in `todo.md` the first bad state, expected field sequence, observed
  field sequence, owning code surfaces, representative tests, and smallest
  focused proof command.

Completion check:

- `todo.md` names the concrete first stdarg bad fact and the code owner to
  repair, without reopening fixed byval placement, HFA/floating, or
  non-HFA aggregate materialization owners without evidence.

### Step 2: Repair Classified Stdarg Owner

Goal: repair the classified AArch64 stdarg field-production owner generally.

Primary target: the generated-code owner localized by Step 1.

Actions:

- Repair the classified owner generally for AArch64 variadic stdarg behavior.
- Keep format traversal, cursor progression, selected payload bytes, and
  observing call operands coherent for repeated aggregate string fields.
- Scope the change to the classified owner; do not add named-case emission
  shortcuts or broad call-boundary replay.
- Preserve scalar `va_arg`, non-HFA aggregate `va_arg`, stdarg cursor,
  `va_start`, HFA/floating returns, fixed-formal publication, sret `x8`, and
  fixed and stack-passed byval behavior.

Completion check:

- Focused proof shows the selected stdarg fields are produced and observed in
  order, with adjacent guardrails still stable.

### Step 3: Add Focused Stdarg Coverage

Goal: make the repaired stdarg owner observable in local backend tests.

Primary target: existing AArch64 semantic/prepared BIR dump tests, machine
printer tests, instruction dispatch tests, or focused `00204.c` dump tests.

Actions:

- Add or extend focused coverage for the repaired owner from Step 2.
- Assert the stdarg field sequence, cursor state, selected bytes, or emitted
  call operands that prove the localized owner.
- Include a smaller local equivalent when possible, but keep the `00204.c`
  dump representative as a guardrail if it exposes the only current shape.
- Preserve adjacent scalar `va_arg`, non-HFA aggregate `va_arg`, stdarg
  cursor, `va_start`, post-`va_arg` call publication, HFA/floating return,
  fixed-formal entry, local/value-home, and frame/formal coverage.

Completion check:

- Local coverage fails without the stdarg repair and passes with the repair.

### Step 4: Validate Representative And Classify Residuals

Goal: prove the stdarg repair on the focused representative and classify any
newly exposed first bad fact.

Primary target: supervisor-selected focused proof scope including `00204.c`.

Actions:

- Run a focused proof including build, local AArch64 backend coverage,
  prior-owner guardrails, and `c_testsuite_aarch64_backend_src_00204_c`.
- Confirm the current stdarg block prints the expected six `ABCDEFGHI` fields
  before the visible `stdarg:` marker.
- Confirm prior stdarg cursor, `va_start`, HFA/floating return, non-HFA
  aggregate `va_arg`, sret `x8`, fixed-formal, local/value-home, and
  frame/formal guardrails remain stable.
- If `00204.c` still fails, classify whether the next first bad fact remains
  in this source idea or belongs to another distinct initiative.
- Record pass/fail results and first bad facts in `todo.md`.

Completion check:

- `todo.md` records fresh proof. The classified stdarg fault is gone,
  prior-owner guardrails remain stable, and any remaining blocker is
  explicitly localized for the next lifecycle decision.
