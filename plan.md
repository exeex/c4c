# AArch64 Variadic Stdarg Cursor Format Residual Runbook

Status: Active
Source Idea: ideas/open/331_aarch64_variadic_stdarg_cursor_format_residual.md
Activated from: idea 326 terminal handoff

## Purpose

Continue from the terminal idea 326 handoff after HFA/floating returns and the
adjacent stdarg/byval aggregate transport repairs advanced. The remaining
representative failure is in the stdarg block, after the first byval payload
bytes are sourced correctly.

## Goal

Classify and repair the current `00204.c` stdarg cursor/format runtime blocker
generally for AArch64 variadic generated code.

## Core Rule

Progress must repair a concrete AArch64 variadic stdarg cursor/format owner.
Do not special-case `00204.c`, `myprintf`, one `%9s` occurrence, one separator
byte, one stack offset, one temporary slot, one format string, one register, or
one emitted instruction sequence.

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
  - runtime output reaches `stdarg:`
  - first byval payload bytes begin as `ABCDEFGHI`
  - separator bytes between the first `%9s` values are corrupted
    (`0xd0`, `0xd4`, `0xd8`, ...)
  - execution segfaults before completing the first stdarg line
- Prior-owner guardrails to preserve:
  - HFA/floating return publication from idea 326
  - sret `x8` transport and large byval indirect pointer transport
  - byval aggregate register-lane allocation and fragmented lane publication
  - non-HFA aggregate `va_arg` source-to-destination copies from idea 330
  - post-`va_arg` call operand publication from idea 329
  - fixed-formal entry publication from idea 327
  - local/value-home publication from idea 325
  - frame/formal coverage from idea 324

## Non-Goals

- Do not reopen HFA/floating argument or return lane publication unless fresh
  generated-code evidence again places the first bad fact there.
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

The remaining failure is after repaired aggregate bytes are consumed by the
first stdarg path. The corrupted separator bytes suggest that the next owner
may be format cursor traversal, variadic cursor progression, register-save-area
or overflow-area state, argument consumption order, or call-boundary
publication around the stdarg loop. The next repair must localize which state
diverges before editing implementation.

## Execution Rules

- Keep routine packet progress in `todo.md`.
- Localize the first stdarg-block divergence before editing code.
- Prefer focused backend coverage for the repaired stdarg owner before relying
  on the external c-testsuite representative.
- Preserve prior-owner guardrails; do not weaken HFA/floating, non-HFA
  aggregate `va_arg`, byval aggregate lane, fixed-formal, local/value-home, or
  frame/formal coverage.
- If `00204.c` advances past the current stdarg cursor/format residual, record
  the next first bad fact in `todo.md` and return it to lifecycle
  classification if it belongs to another owner.

## Ordered Steps

### Step 1: Localize Stdarg Cursor/Format Residual

Goal: identify the first stdarg-block state where the generated code diverges
after the first byval payload begins with the expected bytes.

Primary target: generated `myprintf` stdarg artifacts and AArch64 variadic
cursor, format cursor, payload selection, and call-publication state around
the first malformed `%9s` output.

Actions:

- Trace the expected first stdarg line from source expectation to generated
  format traversal, selected variadic argument payload, separator output, and
  observing call.
- Compare format cursor state, `va_list` fields, register-save-area and
  overflow-area cursors, selected payload homes, and final ABI argument
  publication at the first corrupted separator byte.
- Determine whether the owner is format cursor traversal, `va_list` cursor
  progression, register-save-area or overflow-area state, byval aggregate
  argument consumption order, call-boundary publication, or a distinct
  generated-code path.
- Record in `todo.md` the first bad state, expected value, observed value,
  owning code surfaces, representative tests, and smallest focused proof
  command.

Completion check:

- `todo.md` names the concrete first stdarg cursor/format bad fact and the
  code owner to repair, without reopening HFA/floating or prior aggregate
  materialization owners.

### Step 2: Repair The Classified Stdarg Owner

Goal: make generated AArch64 preserve and publish the correct stdarg cursor,
format, and argument state through the first malformed stdarg output.

Primary target: the generated-code owner localized by Step 1.

Actions:

- Repair the classified owner generally for AArch64 variadic stdarg behavior.
- Preserve format cursor, `va_list` cursor fields, source address,
  destination home, byte count, register class, lane order, and call ordering
  for the selected payload and separator output.
- Include both register-save-area and overflow-area paths when Step 1 shows
  the owner spans both.
- Scope the change to the classified owner; do not add named-case emission
  shortcuts or broad call-boundary replay.
- Preserve scalar `va_arg`, non-HFA aggregate `va_arg`, HFA/floating returns,
  byval aggregate lanes, fixed aggregate calls, and stack-passed large
  aggregate behavior.

Completion check:

- Focused proof shows the selected stdarg payload and separator state are
  published from the correct homes, with adjacent guardrails still stable.

### Step 3: Add Focused Stdarg Coverage

Goal: make the repaired stdarg owner observable in local backend tests.

Primary target: existing AArch64 semantic/prepared BIR dump tests, machine
printer tests, instruction dispatch tests, or focused `00204.c` dump tests.

Actions:

- Add or extend focused coverage for the repaired owner from Step 2.
- Assert cursor source, temporary home, final ABI publication, and following
  call order for the current representative shape or a smaller local
  equivalent.
- Preserve adjacent scalar `va_arg`, non-HFA aggregate `va_arg`,
  post-`va_arg` call publication, fixed aggregate call, byval aggregate lane,
  HFA/floating return, fixed-formal entry, local/value-home, and frame/formal
  coverage.

Completion check:

- Local coverage fails without the stdarg repair and passes with the repair.

### Step 4: Validate Representative And Classify Residuals

Goal: prove the stdarg repair on the focused representative and classify any
newly exposed first bad fact.

Primary target: supervisor-selected focused proof scope including `00204.c`.

Actions:

- Run a focused proof including build, local AArch64 backend coverage,
  prior-owner guardrails, and `c_testsuite_aarch64_backend_src_00204_c`.
- Confirm the first `%9s` payload bytes and separator bytes are correct before
  the observing call.
- Confirm prior HFA/floating return, non-HFA aggregate `va_arg`, byval lane,
  sret `x8`, fixed-formal, local/value-home, and frame/formal guardrails
  remain stable.
- If `00204.c` still fails, classify whether the next first bad fact remains
  in this source idea or belongs to another distinct initiative.
- Record pass/fail results and first bad facts in `todo.md`.

Completion check:

- `todo.md` records fresh proof. The classified stdarg cursor/format fault is
  gone, prior-owner guardrails remain stable, and any remaining blocker is
  explicitly localized for the next lifecycle decision.
