# AArch64 Non-HFA Aggregate Va Arg Materialization Runbook

Status: Active
Source Idea: ideas/open/330_aarch64_non_hfa_aggregate_va_arg_materialization.md
Activated from: idea 329 Step 4 lifecycle handoff

## Purpose

Repair the AArch64 generated-code path where a non-HFA aggregate `va_arg`
source is selected but its bytes are not copied into the temporary aggregate
object before later code observes that object.

## Goal

Ensure non-HFA aggregate `va_arg` materialization copies selected aggregate
bytes from the active `va_list` source into the destination object before
member address publication or ordinary calls can observe the object.

## Core Rule

Progress must be a general AArch64 non-HFA aggregate `va_arg`
materialization capability. Do not special-case `00204.c`, `myprintf`,
`%7s`, `%9s`, `x13`, `sp + 8`, `sp + 15`, `ABCDEFGHI`, `lmnopqr`, one
aggregate size, one branch, one stack offset, or one emitted copy sequence.
Do not reopen idea 329 call-operand publication or idea 326 HFA/floating
lowering unless fresh generated-code evidence proves that surface owns the
current first bad fact.

## Read First

- `ideas/open/330_aarch64_non_hfa_aggregate_va_arg_materialization.md`
- `ideas/open/329_aarch64_variadic_aggregate_va_arg_call_setup.md`
- `ideas/open/326_aarch64_variadic_hfa_floating_residual.md`
- `todo.md`
- generated AArch64 artifacts for `00204.c` under
  `build/c_testsuite_aarch64_backend/src/`
- existing focused `test_after.log`, when present

## Current Targets

- Representative external case:
  - `c_testsuite_aarch64_backend_src_00204_c`
- Current first bad fact:
  - generated `myprintf` now publishes `.str31` / `.str33` into `x0` and the
    aggregate buffer pointer into `x1` before `bl printf`
  - the remaining `%7s` / `%9s` mismatch is corrupted non-HFA stdarg string
    output, because the selected aggregate `va_arg` source bytes are not
    copied into the stack buffers later passed as `x1`
  - branch-local generated code computes or selects the aggregate source
    through `x13`, but the destination buffers at `sp + 8` and `sp + 15` are
    observed without the expected aggregate text bytes
- Prior-owner guardrails to preserve:
  - post-`va_arg` call operand publication from idea 329
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
  materialization issue and fresh evidence reaches those paths.
- Do not reopen post-`va_arg` ordinary-call publication unless generated code
  again reaches a call branch without publishing the fixed format operand and
  aggregate-derived operand into ABI registers.
- Do not change semantic admission, runners, timeout policy, expectations,
  unsupported classifications, CTest registration, or proof-log policy.
- Do not claim completion from classification alone; generated code must copy
  selected non-HFA aggregate bytes into the destination object before the
  following use observes it.

## Working Model

The failing branches are no longer missing final call operands. The following
`printf` calls receive the format string and a pointer to the branch-local
aggregate buffer. The buffer contents are wrong because the aggregate
`va_arg` materialization path selects a source but fails to emit the required
source-to-destination aggregate byte copy before publishing the destination
address.

The repair should bridge non-HFA aggregate `va_arg` source selection to the
temporary aggregate destination for both register-save-area and overflow-area
sources, while preserving later call-operand publication and existing
aggregate/fixed-call guardrails.

## Execution Rules

- Keep routine packet progress in `todo.md`.
- Localize semantic BIR, prepared BIR, `va_list` source selection,
  register-save-area or overflow-area addressing, destination stack object,
  aggregate byte-copy lowering, and generated machine instructions before
  editing code.
- Prefer focused backend coverage for non-HFA aggregate `va_arg`
  materialization before relying on the external c-testsuite representative.
- Preserve prior-owner guardrails; do not weaken post-`va_arg` call
  publication, byval aggregate call-lane publication, fixed-formal entry
  publication, local/value-home publication, frame/formal publication, HFA
  argument/return, global data emission, runner, expectation, or proof-log
  coverage.
- If `00204.c` advances past the non-HFA stdarg string output, record the
  next first bad fact in `todo.md` and return it to lifecycle classification
  if it belongs to another owner.
- Use narrow build plus focused CTest proof for implementation packets.
  Escalate to broader backend validation only when the supervisor requests it
  or the implementation touches shared backend helper/printer behavior
  broadly.

## Ordered Steps

### Step 1: Localize Non-HFA Aggregate Va Arg Materialization

Goal: identify the exact generated-code owner that selects a non-HFA
aggregate `va_arg` source but fails to copy its bytes into the destination
object before the following use.

Primary target: generated `myprintf` artifacts and AArch64 `va_arg`
materialization state for the `%7s` and `%9s` branches in `00204.c`.

Actions:

- Trace each failing branch from `match(&s, "%7s")` and `match(&s, "%9s")`
  through non-HFA aggregate `va_arg` source selection to the branch-local
  destination buffer and following `printf` call.
- Map the semantic aggregate value, prepared aggregate destination, `va_list`
  source, register-save-area or overflow-area address, expected byte count,
  emitted copy or missing copy, and later use that observes the destination.
- Determine whether the missing materialization belongs to source selection,
  destination selection, byte-copy lowering, stack-slot publication,
  instruction ordering, or `va_list` progression.
- Record in `todo.md` the owning code surfaces, representative tests, and the
  smallest focused proof command for the repair.

Completion check:

- `todo.md` names the failing branches, selected source, destination object,
  expected copied bytes, observed emitted instructions, owning code surfaces,
  representative tests, and smallest focused proof command.

### Step 2.1: Localize Missing Ordinary Dispatch Of Prepared Va Arg Memory Operations

Goal: determine why the prepared memory operations for
`vaarg.join.14` / `vaarg.join.39` are not emitted by ordinary block
dispatch before the following call.

Primary target: AArch64 prepared-memory operation retention, ordering, and
ordinary block instruction dispatch for non-HFA aggregate `va_arg`
materialization.

Actions:

- Trace the prepared BIR records for `vaarg.join.14` and `vaarg.join.39`
  from construction through the retained block that contains the later
  `.str31` / `.str33` `printf` calls.
- Identify whether the operations are missing because they are not attached
  to the ordinary block, are attached to a replaced or skipped block, are
  filtered by instruction kind, lose their source/destination address
  operands, or are scheduled after the observing call.
- Compare the prepared-memory operation records against nearby ordinary
  memory operations that are emitted correctly by block dispatch.
- Do not replay all prepared memory accesses at the call boundary; that
  experiment emitted the desired local copy shape but widened into unrelated
  corruption.
- Record the dispatch owner, the skipped operation identity, expected
  source/destination addresses, and the smallest focused proof command in
  `todo.md`.

Completion check:

- `todo.md` identifies the concrete ordinary dispatch break between prepared
  `vaarg.join.14` / `vaarg.join.39` memory records and emitted AArch64
  instructions, without relying on broad call-boundary replay.

### Step 2.2: Repair Ordinary Dispatch For Prepared Va Arg Memory Operations

Goal: make ordinary block dispatch emit the prepared non-HFA aggregate
`va_arg` memory operations in program order before any following use observes
the destination object.

Primary target: the ordinary block dispatch owner localized by Step 2.1.

Actions:

- Repair the ordinary dispatch path so prepared memory operations for
  non-HFA aggregate `va_arg` materialization are retained, visited, and
  lowered in the block that owns the later use.
- Preserve the prepared operation's source address, destination address,
  byte count, and ordering relative to `.str31` / `.str33` call setup.
- Scope the change to the missing dispatch/lowering gap; do not add broad
  call-boundary replay or named-case emission shortcuts.
- Preserve scalar `va_arg`, HFA/floating `va_arg`, ordinary calls, fixed
  aggregate calls, byval aggregate lane publication, and stack-passed large
  aggregate behavior.

Completion check:

- Focused proof shows the `vaarg.join.14` / `vaarg.join.39` prepared memory
  operations are emitted by ordinary block dispatch before the following
  calls, with adjacent publication and aggregate guardrails still stable.

### Step 2.3: Repair Remaining Non-HFA Aggregate Va Arg Copy Semantics If Needed

Goal: emit the required copy from the selected non-HFA aggregate `va_arg`
source into the destination aggregate object before any following use, if
ordinary dispatch is working but the materialized bytes or addresses remain
wrong.

Primary target: AArch64 prepared/codegen lowering for non-HFA aggregate
`va_arg` materialization after Step 2.2 has restored dispatch.

Actions:

- If Step 2.2 restores emission but the copy still uses the wrong source,
  destination, byte count, or order, repair that remaining semantic lowering
  gap.
- Include register-save-area and overflow-area sources when applicable.
- Reuse existing AArch64 stack-slot, aggregate-copy, memory-address,
  register, and temporary-object helpers when available.
- Preserve scalar `va_arg`, HFA/floating `va_arg`, ordinary calls, fixed
  aggregate calls, byval aggregate lane publication, and stack-passed large
  aggregate behavior.

Completion check:

- Focused proof shows non-HFA aggregate `va_arg` values are copied into their
  destination object before following uses without regressing adjacent
  publication and aggregate guardrails.

### Step 3: Add Focused Non-HFA Aggregate Va Arg Materialization Coverage

Goal: make the missing source-to-destination aggregate copy observable in
local backend tests.

Primary target: existing AArch64 semantic/prepared BIR dump tests, machine
printer tests, instruction dispatch tests, or focused `00204.c` dump tests.

Actions:

- Add or extend focused coverage for a non-HFA aggregate `va_arg` whose bytes
  must be copied into a temporary object before a following call or member
  access observes the object.
- Assert the generated or prepared output includes the source selection,
  destination object, expected aggregate copy, and following use in the
  correct order.
- Cover the `%7s` / `%9s` shape or a smaller local equivalent that fails
  without the same owner repair.
- Preserve adjacent scalar `va_arg`, HFA/floating `va_arg`, post-`va_arg`
  call publication, fixed aggregate call, byval aggregate lane,
  fixed-formal entry, local/value-home, and frame/formal coverage.

Completion check:

- Local coverage fails without the non-HFA aggregate `va_arg`
  materialization repair and passes with the repair.

### Step 4: Validate Representative And Classify Residuals

Goal: prove the non-HFA aggregate `va_arg` materialization repair on the
focused representative and classify any newly exposed first bad fact.

Primary target: supervisor-selected focused proof scope including `00204.c`.

Actions:

- Run a focused proof including build, local AArch64 backend coverage,
  prior-owner guardrails, and `c_testsuite_aarch64_backend_src_00204_c`.
- Confirm generated `myprintf` copies selected non-HFA aggregate bytes into
  the `%7s` / `%9s` destination buffers before passing those buffer addresses
  to `printf`.
- Confirm idea 329 call publication remains repaired: format strings are
  published into `x0` and aggregate-derived buffer pointers into following
  ABI argument registers before `bl printf`.
- If `00204.c` still fails, classify whether the next first bad fact belongs
  to idea 326's HFA/floating residual path or another distinct initiative.
- Record pass/fail results and first bad facts in `todo.md`.

Completion check:

- `todo.md` records fresh proof. The non-HFA aggregate `va_arg`
  materialization fault is gone, and any remaining blocker is explicitly
  localized for the next lifecycle decision.
