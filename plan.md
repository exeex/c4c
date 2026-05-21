# AArch64 External Call Symbol Home Publication Plan

Status: Active
Source Idea: ideas/open/354_aarch64_external_call_symbol_home_publication.md
Activated After: ideas/closed/348_aarch64_indexed_aggregate_address_writeback.md

## Purpose

Repair AArch64 generated-code publication for values that must be present in
call arguments, symbol homes, local homes, or preserved stack homes before an
external or direct call consumes them.

Goal: make the current `00187` `RUNTIME_NONZERO` segmentation fault advance
past its call/symbol-home publication failure without reopening indexed
aggregate selected-address/writeback work.

Core Rule: repair a semantic call/symbol-home publication owner; do not
special-case `00187`, one external callee, one symbol, one local, one stack
offset, one register, or one emitted call neighborhood.

## Read First

- `ideas/open/354_aarch64_external_call_symbol_home_publication.md`
- `ideas/closed/348_aarch64_indexed_aggregate_address_writeback.md` for the
  split boundary: `00130`, `00176`, and `00195` now pass, while `00187` remains
  a separate call/symbol-home publication segfault.
- AArch64 call operand publication, prepared value homes, symbol/local home
  publication, preserved stack homes, and emitted direct/external call setup
  before editing.

## Current Targets

- Current representative: `c_testsuite_aarch64_backend_src_00187_c`.
- First bad fact family: external call argument/symbol home publication for a
  value or address that reaches a call boundary stale, unpublished, or
  uninitialized.
- Guardrails: `c_testsuite_aarch64_backend_src_00130_c`,
  `c_testsuite_aarch64_backend_src_00176_c`,
  `c_testsuite_aarch64_backend_src_00195_c`, focused AArch64
  call-publication tests, selected-address/writeback tests, and backend
  guardrails selected by the supervisor.

## Non-Goals

- Do not reopen indexed aggregate selected-address/writeback from idea 348
  unless fresh evidence reaches that exact owner.
- Do not reopen recursive call argument preservation, block label emission
  ordering, formal-to-local frame-slot publication, unsigned div/rem producer
  publication, scalar cast publication, return-result publication,
  variadic/byval/HFA aggregate publication, static initializer
  materialization, semantic admission, runner behavior, timeout policy,
  expectation changes, unsupported classifications, CTest registration, or
  proof-log behavior.
- Do not use filename-only, function-name-only, callee-only, symbol-only,
  stack-offset-only, register-only, c-testsuite-number-specific, or
  emitted-text-only fixes.

## Working Model

- Prepared lowering may assign a live value or address to a symbol, local,
  stack, or register home, but AArch64 call setup must publish that value into
  the ABI-visible argument location or memory home that the callee/runtime path
  consumes.
- A preserved stack home or symbol snapshot is only valid if it is populated
  before the call path reloads or dereferences it.
- If `00187` still fails after a repair, reclassify the new first bad fact
  instead of widening this plan by assumption.

## Execution Rules

- Start from generated/prepared evidence for the current `00187` segfault
  before modifying code.
- Prefer shared publication helpers for call operands, symbol homes, local
  homes, or preserved stack homes over emitted-instruction pattern matching.
- Preserve prior 348 repairs for `00130`, `00176`, and `00195`.
- Treat any fix that recognizes only `00187`, one callee, one symbol, one
  stack slot, one register, or one instruction sequence as route drift.

## Steps

### Step 1: Localize The 00187 Call/Symbol-Home Boundary

Goal: identify the exact prepared-to-AArch64 handoff that leaves a call
operand, symbol home, local home, or preserved stack home unpublished before
`00187` segfaults.

Primary target: prepared value homes, symbol/local publication, preserved
stack homes, call operand assignment, and emitted call setup.

Actions:

- Trace `00187` from prepared MIR/BIR homes through generated call setup to
  the crashing call path.
- Identify whether the bad value is a call argument, symbol address, local
  address, local value, or preserved stack-home reload.
- Record the concrete first bad boundary with generated evidence and the
  focused proof subset for the implementation packet.

Completion check:

- `todo.md` records the localized first bad boundary, representative evidence,
  and the narrow proof command for the repair packet.

### Step 2: Add Focused Call/Symbol-Home Coverage

Goal: prove the publication shape without depending only on `00187`.

Actions:

- Add or identify focused backend coverage for the localized call argument,
  symbol-home, local-home, or preserved-home publication shape.
- Keep assertions semantic: the value or address must be published before the
  call consumer reloads or dereferences it.
- Avoid contracts tied to `00187`, one symbol name, one offset, one register,
  or one emitted instruction neighborhood.

Completion check:

- Focused coverage fails without the repair or an existing focused test is
  identified that already exposes the stale/unpublished call/symbol-home
  behavior.

### Step 3: Repair The Publication Handoff

Goal: make AArch64 publish the live value/address into the call-visible or
memory-visible home before the callee/runtime path consumes it.

Actions:

- Repair the localized shared owner from Step 1.
- Ensure prepared values are not treated as available in stack, symbol, local,
  or argument homes until those homes have actually been filled.
- Preserve selected-address/writeback, formal/local publication, recursive
  call preservation, block labeling, scalar producer, and return behavior.

Completion check:

- Focused coverage from Step 2 passes, and supervisor-selected adjacent
  call-publication and selected-address guardrails show no regression.

### Step 4: Prove 00187 And Reclassify Residuals

Goal: verify `00187` advances past the current segmentation fault and decide
whether this source idea is complete.

Actions:

- Run the supervisor-selected external proof including `00187`.
- Re-run adjacent guardrails including `00130`, `00176`, and `00195`.
- Run the supervisor-chosen broader backend guard once focused proof is
  stable.
- If `00187` remains red, reclassify the new first bad fact instead of
  broadening this plan by assumption.

Completion check:

- `todo.md` records whether `00187` passed, advanced, or exposed a separate
  first bad fact, plus the broader guard result and closure/split
  recommendation.
