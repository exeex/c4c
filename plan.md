# AArch64 Byval Aggregate Register-Lane Publication Runbook

Status: Active
Source Idea: ideas/open/328_aarch64_byval_aggregate_call_argument_lane_publication.md
Reactivated after: ideas/open/331_aarch64_variadic_stdarg_cursor_format_residual.md

## Purpose

Continue the caller-side byval aggregate publication work after the stdarg
route proved the new `00204.c` first bad fact is not inside `myprintf` or
`va_arg` cursor/copy handling. The remaining failure is a split aggregate
call-argument lane materialization/publication gap.

## Goal

Repair AArch64 caller-side publication for small byval aggregate call
arguments whose final ABI register lane is only partially populated, so all
live tail bytes are copied into the lane consumed by call lowering.

## Core Rule

Progress must repair general AArch64 byval aggregate call-argument lane
publication. Do not special-case `00204.c`, `stdarg`, `myprintf`, `%9s`,
`s9`, one stack offset, one aggregate size, one lane number, one register, or
one emitted instruction sequence.

## Read First

- `ideas/open/328_aarch64_byval_aggregate_call_argument_lane_publication.md`
- `todo.md`
- generated AArch64 artifacts for `00204.c` under
  `build/c_testsuite_aarch64_backend/src/`
- existing focused `test_after.log`, when present

## Current Targets

- Representative external case:
  - `c_testsuite_aarch64_backend_src_00204_c`
- Current first bad fact:
  - the first `stdarg:` payload line now prints all six `ABCDEFGHI` fields
  - the second `stdarg:` payload line should print
    `lmnopqr ABCDEFGHI ABCDEFGHI ABCDEFGHI ABCDEFGHI ABCDEFGHI`
  - the observed line prints `lmnopqr ABCDEFGH ...`, dropping the ninth byte
    from each `%9s` aggregate before `myprintf` observes it
- Current owner classification:
  - semantic/prepared BIR for caller `stdarg` carries the ninth byte for each
    `%9s` aggregate
  - generated assembly stores that byte to separate temporary slots such as
    `[sp,#8132]`
  - emitted high byval lanes are loaded from unpopulated aggregate lane slots
    such as `[sp,#112]`, `[sp,#128]`, `[sp,#144]`, `[sp,#160]`, and `[sp,#176]`
  - `myprintf` cursor progression, aggregate `va_arg` copy count, destination
    buffering, and format traversal are downstream observers, not first owners
- Likely repair surfaces:
  - `src/backend/bir/lir_to_bir/memory/local_slots.cpp`
  - `src/backend/mir/aarch64/codegen/calls.cpp`
  - helpers around `call_arg_byval_aggregate_register_lanes`,
    `make_byval_register_lane_prepared_source`, and fragmented aggregate
    register lane publication
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

- Do not reopen stdarg format traversal, `va_list` cursor progression,
  aggregate `va_arg` copy byte count, or `myprintf` destination buffering
  unless fresh generated-code evidence moves the first bad fact there.
- Do not reopen fixed non-HFA byval rounded slot placement or the coherent
  register-to-stack transition unless generated-code evidence shows the
  current first bad fact is placement rather than lane materialization.
- Do not reopen HFA/floating publication, fixed-formal entry publication,
  local/value-home publication, frame/formal publication, runner behavior,
  expectation files, unsupported classifications, timeout policy, CTest
  registration, or proof-log policy without direct generated-code evidence.

## Working Model

The call plan for the failing `stdarg` call is credible: `%7s` consumes `x1`,
and each `%9s` aggregate is a two-register byval argument through available
GPR lanes and then stack. The split aggregate tail byte exists in prepared
state, but the call boundary loads the high ABI lane from a slot that was not
populated with that tail byte. The repair should connect partial upper-lane
aggregate loads/stores to the byval lane source consumed by call publication.

## Execution Rules

- Keep routine packet progress in `todo.md`.
- Repair lane publication semantically, not by recognizing the representative
  format string or source variable names.
- Prefer focused backend coverage for partial final-lane byval aggregate
  publication before relying on the external c-testsuite representative.
- Preserve prior-owner guardrails; do not weaken stdarg cursor, `va_start`,
  HFA/floating, non-HFA aggregate `va_arg`, fixed-formal, local/value-home, or
  frame/formal coverage.
- If `00204.c` advances past this residual, record the next first bad fact in
  `todo.md` and return it to lifecycle classification if it belongs to another
  owner.

## Ordered Steps

### Step 1: Repair Partial Upper-Lane Byval Publication

Goal: make AArch64 call lowering publish live bytes for a byval aggregate's
partially populated final register lane.

Primary targets:

- `src/backend/bir/lir_to_bir/memory/local_slots.cpp`
- `src/backend/mir/aarch64/codegen/calls.cpp`

Actions:

- Trace the current local-slot to byval-lane handoff for split aggregates whose
  final ABI lane has fewer than eight live bytes.
- Identify why the live tail byte is stored to a temporary slot while the
  emitted high lane is loaded from an unpopulated aggregate lane slot.
- Repair the general handoff so the byval lane source consumed by call
  publication contains the live tail bytes for sizes 9 through 15 and any
  adjacent partial-lane shape covered by the same helper.
- Preserve existing behavior for single-lane small aggregates, full two-lane
  aggregates, large stack-passed byval aggregates, and rounded fixed byval
  placement across the register-to-stack transition.

Completion check:

- Generated code for the representative call publishes the ninth byte into the
  second ABI lane before the call, without changing unrelated call-boundary
  placement.

### Step 2: Add Focused Lane-Publication Coverage

Goal: make the repaired partial-lane publication observable in local backend
tests.

Primary target: existing AArch64 prepared-BIR, machine-printer, instruction
dispatch, or focused `00204.c` dump coverage.

Actions:

- Add or extend focused coverage for a small byval aggregate whose final
  register lane is partially populated.
- Assert source-byte materialization and call-boundary publication into the
  ABI lane consumed before `bl`.
- Include at least one shape that proves a multi-register aggregate tail byte
  is not stranded in a temporary slot.
- Keep larger stack-passed byval aggregates and rounded byval placement covered
  as guardrails.

Completion check:

- Focused coverage fails without the lane-publication repair and passes with
  it.

### Step 3: Validate Representative And Classify Residuals

Goal: prove the partial-lane publication repair on focused guardrails and the
`00204.c` representative.

Primary target: supervisor-selected focused proof scope including `00204.c`.

Actions:

- Run a focused proof including build, local AArch64 backend coverage,
  prior-owner guardrails, and `c_testsuite_aarch64_backend_src_00204_c`.
- Confirm the second `stdarg:` payload line keeps the ninth byte in each `%9s`
  argument before `myprintf` observes it.
- Confirm prior byval rounded placement, `va_start`, stdarg cursor,
  HFA/floating, non-HFA aggregate `va_arg`, fixed-formal, local/value-home,
  and frame/formal guardrails remain stable.
- If `00204.c` still fails, classify whether the next first bad fact remains
  in this source idea or belongs to another distinct initiative.
- Record pass/fail results and first bad facts in `todo.md`.

Completion check:

- `todo.md` records fresh proof. The partial upper-lane publication fault is
  gone, prior-owner guardrails remain stable, and any remaining blocker is
  explicitly localized for the next lifecycle decision.
