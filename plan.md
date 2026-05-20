# AArch64 Byval Aggregate Call Argument Placement Runbook

Status: Active
Source Idea: ideas/open/328_aarch64_byval_aggregate_call_argument_lane_publication.md
Reactivated after: ideas/open/326_aarch64_variadic_hfa_floating_residual.md

## Purpose

Continue from the idea 326 handoff after the HFA/floating residual was
repaired. The representative now fails in fixed non-HFA byval aggregate
argument placement across the AArch64 call boundary.

## Goal

Repair AAPCS64 byval aggregate argument placement so each fixed non-HFA
aggregate consumes its own rounded register or stack slots, and caller
publication matches callee entry unpack across register-lane and stack
transitions.

## Core Rule

Progress must repair general AArch64 byval aggregate call-boundary behavior.
Do not special-case `00204.c`, `arg`, `fa1`, `s8` through `s13`, one aggregate
size, one register, one stack offset, one string fragment, or one emitted
instruction sequence.

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
  - runtime mismatch is in the fixed `Arguments:` section after the repaired
    HFA/floating block
  - `fa1(struct s8..s13)` should print `stu ABC JKL TUV 456 ghi`
  - observed output is `stu ABC I JKL RS TUV`
  - generated BIR materializes bytes from `s8` through `s13` correctly
  - generated caller publishes contiguous chunks through `x0` through `x6`
  - generated callee entry treats `x0` through `x5` as six independent byval
    formal starts, shifting later aggregates onto trailing chunks
- Prior-owner guardrails to preserve:
  - AArch64 `va_start` overflow cursor initialization from idea 331
  - HFA/floating argument and return publication from idea 326
  - sret `x8` transport and large byval indirect pointer transport
  - non-HFA aggregate `va_arg` source-to-destination copies from idea 330
  - post-`va_arg` call operand publication from idea 329
  - fixed-formal entry publication from idea 327
  - local/value-home publication from idea 325
  - frame/formal coverage from idea 324

## Non-Goals

- Do not reopen HFA/floating argument or return publication unless fresh
  generated-code evidence again places the first bad fact there.
- Do not reopen stdarg cursor/format or `va_start` overflow cursor
  initialization unless fresh generated-code evidence again places the first
  bad fact there.
- Do not reopen non-HFA aggregate string `va_arg` materialization for `%7s` or
  `%9s` unless selected aggregate bytes are again missing before their
  observing calls.
- Do not rewrite fixed-formal entry publication, local/value-home publication,
  frame/formal publication, runner behavior, expectation files, unsupported
  classifications, timeout policy, CTest registration, or proof-log policy
  without direct generated-code evidence.

## Working Model

The semantic source bytes are correct before ABI lowering. The failure appears
when byval aggregate arguments are flattened into a continuous stream of GPR
chunks while callee entry unpacks each formal as if it begins at the next
register. AAPCS64 requires each non-HFA aggregate argument to consume its own
rounded slots; if the whole aggregate cannot fit in the remaining argument
registers, caller and callee must agree on stack placement for that aggregate
and following arguments.

## Execution Rules

- Keep routine packet progress in `todo.md`.
- Localize caller planning, caller publication, and callee entry unpack before
  editing code.
- Prefer focused backend coverage for the repaired byval aggregate placement
  owner before relying on the external c-testsuite representative.
- Preserve prior-owner guardrails; do not weaken HFA/floating, stdarg cursor,
  `va_start`, non-HFA aggregate `va_arg`, fixed-formal, local/value-home, or
  frame/formal coverage.
- If `00204.c` advances past the current byval aggregate residual, record the
  next first bad fact in `todo.md` and return it to lifecycle classification if
  it belongs to another owner.

## Ordered Steps

### Step 1: Localize Byval Aggregate Slot Accounting Residual

Goal: identify the first state where caller and callee disagree about byval
aggregate slot starts for `fa1(s8, s9, s10, s11, s12, s13)`.

Primary target: prepared call plans, prealloc/register allocation records,
caller byval publication, and AArch64 entry formal unpack for fixed non-HFA
aggregates.

Actions:

- Trace `%t103` through `%t108` from generated BIR source bytes into prepared
  call facts, call-plan argument locations, emitted caller registers or stack
  slots, and generated `fa1` entry homes.
- Determine where the continuous chunk stream diverges from per-aggregate
  rounded slot accounting.
- Classify whether the owner is `src/backend/prealloc/call_plans.cpp`,
  `src/backend/prealloc/regalloc.cpp`,
  `src/backend/mir/aarch64/codegen/calls.cpp`,
  `src/backend/mir/aarch64/codegen/dispatch.cpp`, or a narrower helper under
  those surfaces.
- Record in `todo.md` the first bad state, expected placement, observed
  placement, owning code surfaces, representative tests, and smallest focused
  proof command.

Completion check:

- `todo.md` names the concrete first byval aggregate placement bad fact and
  the code owner to repair, without reopening semantic byte materialization or
  HFA/floating owners.

### Step 2: Repair Caller And Callee Byval Placement Agreement

Goal: make caller publication and callee entry unpack use the same AAPCS64
byval aggregate slot accounting.

Primary target: the generated-code owner localized by Step 1.

Actions:

- Repair the classified owner generally for fixed non-HFA byval aggregate
  arguments.
- Ensure each aggregate consumes its own rounded register slots, with coherent
  stack transition when not enough GPR argument registers remain.
- Keep caller publication and callee entry materialization aligned for
  one-register, multi-register, and stack-transition aggregates.
- Scope the change to the classified owner; do not add named-case emission
  shortcuts or broad call-boundary replay.
- Preserve scalar `va_arg`, non-HFA aggregate `va_arg`, stdarg cursor,
  `va_start`, HFA/floating returns, fixed-formal publication, sret `x8`, and
  large byval indirect pointer behavior.

Completion check:

- Focused proof shows the selected fixed aggregate arguments are published and
  unpacked from matching homes, with adjacent guardrails still stable.

### Step 3: Add Focused Byval Placement Coverage

Goal: make the repaired byval aggregate placement owner observable in local
backend tests.

Primary target: existing AArch64 semantic/prepared BIR dump tests, machine
printer tests, instruction dispatch tests, or focused `00204.c` dump tests.

Actions:

- Add or extend focused coverage for the repaired owner from Step 2.
- Assert per-aggregate rounded slot starts, final ABI register or stack
  publication, and entry materialization for the current representative shape
  or a smaller local equivalent.
- Include a case where an aggregate cannot fit in the remaining GPR argument
  registers and must transition coherently to stack passing.
- Preserve adjacent scalar `va_arg`, non-HFA aggregate `va_arg`, stdarg
  cursor, `va_start`, post-`va_arg` call publication, HFA/floating return,
  fixed-formal entry, local/value-home, and frame/formal coverage.

Completion check:

- Local coverage fails without the byval placement repair and passes with the
  repair.

### Step 4: Validate Representative And Classify Residuals

Goal: prove the byval aggregate placement repair on the focused representative
and classify any newly exposed first bad fact.

Primary target: supervisor-selected focused proof scope including `00204.c`.

Actions:

- Run a focused proof including build, local AArch64 backend coverage,
  prior-owner guardrails, and `c_testsuite_aarch64_backend_src_00204_c`.
- Confirm `fa1(struct s8..s13)` prints `stu ABC JKL TUV 456 ghi`.
- Confirm prior stdarg cursor, `va_start`, HFA/floating return, non-HFA
  aggregate `va_arg`, sret `x8`, fixed-formal, local/value-home, and
  frame/formal guardrails remain stable.
- If `00204.c` still fails, classify whether the next first bad fact remains
  in this source idea or belongs to another distinct initiative.
- Record pass/fail results and first bad facts in `todo.md`.

Completion check:

- `todo.md` records fresh proof. The classified byval aggregate placement
  fault is gone, prior-owner guardrails remain stable, and any remaining
  blocker is explicitly localized for the next lifecycle decision.
