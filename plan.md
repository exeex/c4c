# AArch64 Scalar Cast Register-Source Operand Facts Runbook

Status: Active
Source Idea: ideas/open/338_aarch64_scalar_cast_register_source_operand_facts.md

## Purpose

Repair the selected scalar cast path so AArch64 `sign_extend` and
`zero_extend` machine nodes reach printing with a structured register source
operand.

## Goal

Advance the focused 9-case post-337 scalar cast bucket past the current
machine-printer diagnostic without reopening closed owners from counts alone.

## Core Rule

Fix the semantic operand-fact publication path for scalar casts. Do not use
c-testsuite filename matching, diagnostic-string matching, expectation
changes, runner changes, timeout changes, unsupported rewrites, or CTest
registration changes to claim progress.

## Read First

- `ideas/open/338_aarch64_scalar_cast_register_source_operand_facts.md`
- `ideas/open/295_backend_regex_failure_family_inventory.md`
- Current representative bucket:
  - `c_testsuite_aarch64_backend_src_00143_c`
  - `c_testsuite_aarch64_backend_src_00173_c`
  - `c_testsuite_aarch64_backend_src_00175_c`
  - `c_testsuite_aarch64_backend_src_00176_c`
  - `c_testsuite_aarch64_backend_src_00181_c`
  - `c_testsuite_aarch64_backend_src_00185_c`
  - `c_testsuite_aarch64_backend_src_00204_c`
  - `c_testsuite_aarch64_backend_src_00205_c`
  - `c_testsuite_aarch64_backend_src_00216_c`

## Current Targets

- AArch64 selected scalar cast nodes with `sign_extend` and `zero_extend`
  opcodes.
- The producer, normalizer, selected-node handoff, or printer boundary that
  should carry a register source operand for scalar casts.
- Focused backend coverage for scalar cast selected-node operand facts.

## Non-Goals

- Do not reopen closed ideas 334 through 337 from pass/fail counts alone.
- Do not broaden into runtime value correctness, timeout/output-storm,
  aggregate, pointer, variadic, frame-layout, or semantic `lir_to_bir`
  owners unless this idea first moves the scalar cast diagnostic to a new
  first bad fact and records that evidence.
- Do not weaken expectations, unsupported classifications, runner behavior,
  timeout policy, proof-log policy, or CTest registration.
- Do not hard-code any c-testsuite filename, diagnostic text, source line,
  register number, or emitted instruction sequence.

## Working Model

The current first bad fact is compile-stage AArch64 machine printing. Selected
scalar cast nodes are present, but their register source operand is missing or
not represented in the structured form the printer accepts. The likely repair
surface is before or at selected-node operand fact publication, not in runtime
result comparison.

## Execution Rules

- Keep packet progress, proof commands, and any new first bad facts in
  `todo.md`.
- Prefer narrow focused backend tests before rerunning the full 9-case
  external bucket.
- When touching shared selected-node or machine-printer behavior, include
  nearby backend coverage that would catch scalar arithmetic operand-fact
  regressions.
- If the current diagnostic disappears but representatives still fail, record
  the new first bad fact before proposing closure.

## Steps

### Step 1: Localize Scalar Cast Operand Fact Loss

Goal: identify the exact boundary where scalar cast nodes lose the structured
register source operand required by the AArch64 printer.

Primary target: selected `sign_extend` and `zero_extend` machine-node
construction and printing.

Actions:

- Inspect the selected-node and AArch64 printer path for scalar cast nodes.
- Compare a failing representative with focused backend tests that already
  exercise printable register-sourced scalar nodes.
- Determine whether the missing source belongs to selection, operand
  normalization, selected-node fact storage, or printer consumption.
- Record the boundary and evidence in `todo.md`.

Completion check:

- `todo.md` names the first bad internal boundary and the representative proof
  command used to observe it.

### Step 2: Repair Register-Source Fact Publication

Goal: ensure selected scalar cast nodes carry a structured register source
operand into AArch64 printing.

Primary target: the boundary localized in Step 1.

Actions:

- Implement the smallest general repair for register-sourced scalar
  `sign_extend` and `zero_extend` operand facts.
- Preserve existing scalar arithmetic, immediate, memory, and call-boundary
  selected-node contracts.
- Add or update focused backend coverage for register-sourced scalar cast
  selected nodes.
- Avoid c-testsuite-number, diagnostic-string, or one-opcode-only shortcuts.

Completion check:

- Focused backend coverage fails without the repair and passes with it, and no
  existing focused backend scalar operand tests regress.

### Step 3: Prove The 9-Case Bucket

Goal: verify that the focused external bucket advances past the current
machine-printer diagnostic.

Primary target: the 9 `c_testsuite_aarch64_backend_src_*` representatives
listed in this runbook.

Actions:

- Run the supervisor-delegated focused CTest subset for the 9 representatives
  and relevant backend tests.
- Confirm the old structured register source operand diagnostic is absent.
- Classify any remaining failures by first bad fact rather than treating the
  bucket as automatically complete.
- Record proof results and residual classification in `todo.md`.

Completion check:

- The 9-case bucket no longer fails with the scalar cast structured
  register-source operand diagnostic, and any remaining residuals are
  classified.

### Step 4: Closure Review And Regression Proof

Goal: prepare this focused owner for lifecycle close or a documented follow-on
split.

Primary target: focused proof plus any supervisor-selected broader guard.

Actions:

- Run the supervisor-selected regression or broader backend subset after the
  focused proof is green enough for acceptance.
- Compare the final diff against this source idea's reviewer reject signals.
- If the scalar cast first bad fact moved to a distinct owner, record the
  follow-on candidate in `todo.md` rather than broadening this idea.

Completion check:

- The focused owner is either ready for plan-owner close with matching proof
  logs, or `todo.md` records the blocker and the next focused owner boundary.
