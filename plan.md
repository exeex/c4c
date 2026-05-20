# AArch64 Duff Fallthrough Copy Fixed-Offset Skip Runbook

Status: Active
Source Idea: ideas/open/343_aarch64_duff_fallthrough_copy_fixed_offset_skip.md
Activated From: ideas/closed/342_aarch64_duff_do_while_latch_condition_emission.md residual split

## Purpose

Repair the generated AArch64 residual where Duff fallthrough copy blocks skip
every other fixed short-copy offset after the first fallthrough case.

## Goal

Make generated AArch64 emit the consecutive fixed offsets recorded in prepared
BIR for Duff-style short-copy fallthrough chains.

## Core Rule

Fix the prepared-BIR-to-machine or generated-AArch64 offset propagation
generally. Do not special-case `00143`, `.LBB1_8`, Duff-device labels, block
numbers, stack offsets, source lines, or emitted instruction strings.

## Read First

- `ideas/open/343_aarch64_duff_fallthrough_copy_fixed_offset_skip.md`
- `ideas/closed/342_aarch64_duff_do_while_latch_condition_emission.md`
  completion note for split evidence
- Current residual evidence:
  - idea 342 repaired the duplicate latch decrement
  - prepared BIR for `00143` contains consecutive Duff copy offsets, including
    `+2 -> +2`, `+4 -> +4`, and onward
  - generated AArch64 emits the first fallthrough copy, then skips every other
    short-copy offset
  - example: `.LBB1_8` copies `[sp,#4] -> [sp,#82]` where prepared BIR expects
    the `+2 -> +2` copy

## Current Targets

- Primary representative:
  - `c_testsuite_aarch64_backend_src_00143_c`
- Useful narrow probes:
  - `ctest --test-dir build -j --output-on-failure -R 'c_testsuite_aarch64_backend_src_00143_c'`
  - `./build/c4cll --dump-prepared-bir --target aarch64-linux-gnu --mir-focus-function main tests/c/external/c-testsuite/src/00143.c`
  - `./build/c4cll --codegen asm --target aarch64-linux-gnu tests/c/external/c-testsuite/src/00143.c -o /tmp/c4c_00143_offsets.s`
- Focused backend coverage should exercise generated AArch64 fallthrough copy
  offsets for a short-copy chain whose prepared BIR offsets advance by two
  bytes per case.

## Non-Goals

- Do not edit expectation, unsupported, runner, timeout, CTest registration, or
  proof-log policy.
- Do not reopen Duff latch-condition decrement/compare emission unless fresh
  evidence shows the duplicate latch decrement returned.
- Do not reopen scalar-cast source-publication work unless the old structured
  register-source diagnostic returns.
- Do not broaden into frame layout, aggregate, variadic, parser, semantic HIR,
  or frontend rewrites without fresh first-bad-fact evidence.

## Working Model

Prepared BIR still has the intended fixed-offset copy chain, but the generated
AArch64 fallthrough labels advance selected stack offsets by four bytes per
case after the first fallthrough entry. The likely owner is between prepared
memory operand records, selected memory operand construction, case/fallthrough
scheduling, block emission ordering, or printer input for fixed local offsets.

## Execution Rules

- Keep routine localization and proof notes in `todo.md`.
- Add focused backend coverage before or with the repair when the fixed-offset
  skip can be expressed without relying only on `00143`.
- Preserve idea 342's corrected latch emission: the latch must not branch on a
  second decremented counter value.
- Reclassify any remaining `00143` residual by prepared-state,
  generated-code, assembler/linker output, or runtime evidence before claiming
  completion.

## Steps

### Step 1: Localize Fixed-Offset Skip Boundary

Goal: identify where consecutive prepared Duff copy offsets become generated
AArch64 offsets that skip every other short-copy slot.

Primary target: the `00143` fallthrough copy chain where prepared BIR records
`+2 -> +2` but generated `.LBB1_8` emits `[sp,#4] -> [sp,#82]`.

Actions:

- Reproduce the `00143` runtime residual and regenerate prepared BIR plus
  AArch64 assembly probes.
- Trace the fallthrough copy offsets from prepared memory operand records
  through lowering, selected memory operand construction, block scheduling,
  emission, and machine printer inputs.
- Identify whether the skip is caused by operand sizing, offset scaling,
  fallthrough-case indexing, block ordering, local slot addressing, or another
  concrete backend boundary.
- Record the concrete first bad owner and proposed repair boundary in
  `todo.md`.

Completion check:

- `todo.md` names the concrete lowering, selection, scheduling, addressing, or
  emission boundary where consecutive prepared offsets first become skipped
  generated offsets.

### Step 2: Repair Fallthrough Copy Fixed-Offset Emission

Goal: make generated AArch64 emit consecutive fixed offsets for Duff short-copy
fallthrough chains.

Primary target: the boundary localized in Step 1.

Actions:

- Implement the semantic repair without matching `00143`, `.LBB` names, block
  numbers, stack offsets, source lines, or instruction strings.
- Add or update focused backend coverage for short-copy fallthrough fixed
  offsets.
- Preserve the repaired do-while latch condition behavior from idea 342 and
  unrelated local storage/writeback behavior.

Completion check:

- Focused backend coverage proves generated AArch64 does not skip every other
  short-copy offset when prepared BIR records consecutive two-byte offsets.

### Step 3: Prove Representative And Reclassify Residuals

Goal: verify the fixed-offset skip is repaired and classify any new first bad
fact.

Primary target: `c_testsuite_aarch64_backend_src_00143_c`.

Actions:

- Run the supervisor-delegated focused proof command.
- Confirm generated AArch64 for the fallthrough copy chain includes the
  expected consecutive short-copy offsets.
- Confirm the Duff latch still uses the single post-decrement counter value.
- If `00143` still fails, classify the new first bad fact from prepared BIR,
  generated assembly, assembler/linker output, or runtime output.
- Update `todo.md` with proof results and any residual owner recommendation.

Completion check:

- The focused proof either passes `00143` or records a new first bad fact that
  is outside Duff fallthrough copy fixed-offset emission.
