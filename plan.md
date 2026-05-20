# AArch64 Duff Do-While Latch Condition Emission Runbook

Status: Active
Source Idea: ideas/open/342_aarch64_duff_do_while_latch_condition_emission.md
Activated From: ideas/closed/341_aarch64_fallthrough_fixed_offset_local_load_store_emission.md residual split

## Purpose

Repair the generated-code residual where the Duff-device do-while latch uses a
second decrement in the branch decision path after the fixed-offset fallthrough
copy emission was repaired.

## Goal

Make AArch64 emission branch on the single post-decrement Duff loop counter
value, not on a freshly subtracted second `n - 1` value.

## Core Rule

Fix the latch-condition lowering or emission path generally. Do not special-case
`00143`, `.LBB1_29`, Duff-device labels, block numbers, local name `n`, source
lines, or emitted instruction strings.

## Read First

- `ideas/open/342_aarch64_duff_do_while_latch_condition_emission.md`
- `ideas/closed/341_aarch64_fallthrough_fixed_offset_local_load_store_emission.md`
  completion note for split evidence
- Current residual evidence:
  - fixed-offset fallthrough copy labels `.LBB1_8` through `.LBB1_20` now emit
    `ldrh`/`strh` data movement
  - `00143` still runs and fails as `[RUNTIME_NONZERO]`
  - generated `.LBB1_29` stores `--n`, then reloads `n`, subtracts one again,
    and branches on that second `n - 1` compare
  - the double-decrement branch decision causes one fewer Duff-loop iteration

## Current Targets

- Primary representative:
  - `c_testsuite_aarch64_backend_src_00143_c`
- Useful narrow probes:
  - `ctest --test-dir build -j --output-on-failure -R 'c_testsuite_aarch64_backend_src_00143_c'`
  - `./build/c4cll --dump-prepared-bir --target aarch64-linux-gnu --mir-focus-function main tests/c/external/c-testsuite/src/00143.c`
  - `./build/c4cll --codegen asm --target aarch64-linux-gnu tests/c/external/c-testsuite/src/00143.c -o /tmp/c4c_00143_latch.s`
- Focused backend coverage should exercise a decrementing do-while latch where
  the stored post-decrement value is also the branch condition value.

## Non-Goals

- Do not edit expectation, unsupported, runner, timeout, CTest registration, or
  proof-log policy.
- Do not reopen fixed-offset fallthrough copy emission unless generated
  fallthrough copy blocks again omit required data movement.
- Do not reopen scalar-cast source-publication work unless the old structured
  register-source diagnostic returns.
- Do not broaden into frame layout, aggregate, variadic, parser, semantic HIR,
  or frontend rewrites without fresh first-bad-fact evidence.

## Working Model

The remaining representative failure is no longer in the fallthrough copy
blocks. Generated AArch64 reaches the latch with copied data movement present,
but the latch condition materializes two decrements: one stored as `--n`, then a
second subtract on the reloaded `n` for the compare/branch. The likely owner is
between latch-condition lowering, selected compare construction, branch
emission, condition materialization, or local writeback reuse.

## Execution Rules

- Keep routine localization and proof notes in `todo.md`.
- Add focused backend coverage before or with the repair when the do-while latch
  behavior can be expressed without relying only on `00143`.
- Preserve the fixed-offset fallthrough copy boundary from idea 341 and the
  scalar-cast boundary from idea 340 unless fresh evidence moves the first bad
  fact back there.
- Reclassify any remaining `00143` residual by prepared-state,
  generated-code, assembler/linker output, or runtime evidence before claiming
  completion.

## Steps

### Step 1: Localize Duplicate Latch Decrement Boundary

Goal: identify where the Duff do-while latch branches on a second decremented
value instead of the stored post-decrement value.

Primary target: generated `.LBB1_29` in `00143`, where `--n` is stored, `n` is
reloaded, decremented again, and used for the branch condition.

Actions:

- Reproduce the `00143` runtime residual and regenerate prepared BIR plus
  AArch64 assembly probes.
- Trace the latch counter update and branch condition from prepared BIR through
  lowering, selected-node construction, compare/branch selection, scheduling,
  local writeback, and printer emission.
- Identify whether the duplicate decrement is caused by condition lowering,
  selected compare construction, branch emission, local writeback reuse, or
  another concrete backend boundary.
- Record the concrete first bad owner and proposed repair boundary in
  `todo.md`.

Completion check:

- `todo.md` names the concrete lowering, compare, branch, writeback, or
  emission boundary where the second latch decrement is introduced.

### Step 2: Repair Do-While Latch Condition Emission

Goal: make emitted AArch64 use the single post-decrement counter value for the
do-while latch branch.

Primary target: the boundary localized in Step 1.

Actions:

- Implement the semantic repair without matching `00143`, `.LBB1_29`, label
  names, block numbers, local names, source lines, or instruction strings.
- Add or update focused backend coverage for decrementing do-while latch
  condition emission.
- Preserve fixed-offset fallthrough copy emission behavior and unrelated local
  storage/writeback behavior.

Completion check:

- Focused backend coverage proves a decrementing do-while latch does not branch
  on a second decremented counter value.

### Step 3: Prove Representative And Reclassify Residuals

Goal: verify the duplicate latch decrement is repaired and classify any new
first bad fact.

Primary target: `c_testsuite_aarch64_backend_src_00143_c`.

Actions:

- Run the supervisor-delegated focused proof command.
- Confirm generated AArch64 for the latch does not store `--n` and then branch
  on a second `n - 1` compare.
- If `00143` still fails, classify the new first bad fact from prepared BIR,
  generated assembly, assembler/linker output, or runtime output.
- Update `todo.md` with proof results and any residual owner recommendation.

Completion check:

- The focused proof either passes `00143` or records a new first bad fact that
  is outside Duff do-while latch condition emission.
