# AArch64 MOVI Zero-Extension Materialization Runbook

Status: Active
Source Idea: ideas/open/332_aarch64_movi_zero_extension_materialization.md
Split from: ideas/open/326_aarch64_variadic_hfa_floating_residual.md

## Purpose

Continue the AArch64 `00204.c` representative after commit `b5975e0cd`
repaired the HFA overflow assignment and scalar-FP symbol-load placement
owners. The new first bad fact is later in MOVI integer output, not remaining
HFA/floating or byval publication.

## Goal

Classify and repair the generated-code owner that sign-extends MOVI integer
payloads where expected output requires zero-extension.

## Core Rule

Repair a general AArch64 integer materialization, extension, or call-output
transport capability. Do not special-case `00204.c`, the MOVI block, one
literal, one output line, one register, one stack slot, or one emitted
instruction sequence.

## Read First

- `ideas/open/332_aarch64_movi_zero_extension_materialization.md`
- `todo.md`
- generated AArch64 artifacts for `00204.c` under
  `build/c_testsuite_aarch64_backend/src/`
- existing focused `test_after.log`, when present

## Current Targets

- Representative external case:
  - `c_testsuite_aarch64_backend_src_00204_c`
- Current first bad fact:
  - expected `abcd0000`
  - actual `ffffffffabcd0000`
- Nearby pattern:
  - expected `aaaaaaaa`
  - actual `ffffffffaaaaaaaa`
- Current owner classification:
  - HFA long-double, double, and float sections are repaired
  - byval aggregate payload helpers and prepared handoff dump are guardrails
  - the active owner is MOVI integer zero-extension or materialization

## Non-Goals

- Do not reopen HFA/floating argument, return, overflow, or symbol-load
  placement repairs unless fresh generated-code evidence again shows HFA
  corruption.
- Do not reopen byval aggregate lane publication, aggregate `va_arg`, stdarg
  cursor progression, fixed-formal entry publication, local/value-home
  publication, frame/formal publication, or large frame layout without direct
  generated-code evidence.
- Do not change semantic admission, runners, timeout policy, expectations,
  unsupported classifications, proof-log policy, or CTest registration.

## Working Model

The representative now reaches MOVI after the repaired variadic HFA and byval
sections. The observed values preserve the low 32 bits but fill the high 32
bits with ones, so the next packet should localize where a value that should
be zero-extended is treated as signed or materialized through a sign-extending
path before the observing output call.

## Execution Rules

- Keep routine packet progress in `todo.md`.
- Start from generated-code evidence for the MOVI first bad fact before
  editing implementation.
- Preserve the repaired HFA overflow assignment, scalar-FP symbol-load
  placement, byval aggregate lane publication, stdarg cursor, fixed-formal,
  and local/value-home publication guardrails.
- Prefer focused backend coverage for the localized MOVI or integer
  materialization owner before relying on the external c-testsuite
  representative.
- If `00204.c` advances past this residual, record the next first bad fact in
  `todo.md` and return it to lifecycle classification if it belongs to another
  owner.

## Ordered Steps

### Step 1: Localize The MOVI Sign-Extension Owner

Goal: identify the first generated-code owner that turns the expected MOVI
zero-extended payload into a sign-extended runtime value.

Primary targets:

- generated AArch64 assembly and dumps for `00204.c`
- AArch64 integer immediate materialization, extension, move, and call-output
  argument lowering paths implicated by the generated evidence

Actions:

- Map the MOVI expected and actual output values back to source-level
  expressions and generated temporaries.
- Inspect prepared BIR and generated assembly for the producing value, any
  intermediate extension or move, and the observing output call.
- Determine whether source width, signedness, immediate construction,
  register width, or call argument materialization first introduces the
  high-bit sign fill.
- Record the concrete owner, source value, prepared type or width, ABI lane or
  register, and emitted instruction sequence in `todo.md`.

Completion check:

- `todo.md` names the first owner with generated-code evidence and separates
  it from HFA/floating, byval, stdarg cursor, fixed-formal, and local
  publication owners.

### Step 2: Repair The Classified Owner

Goal: repair the localized AArch64 MOVI or integer materialization path
generally.

Primary target: the backend lowering files identified by Step 1.

Actions:

- Apply the narrow semantic repair for the classified owner.
- Preserve existing guardrails for HFA overflow assignment, scalar-FP
  symbol-load placement, byval aggregate lane publication, aggregate `va_arg`,
  `va_start`, fixed-formal entry publication, local/value-home publication,
  and frame/formal publication.
- Avoid testcase-shaped matching on the representative source, MOVI output
  line, value literals, registers, stack offsets, or printed assembly.

Completion check:

- Generated code materializes or transports the MOVI values with the expected
  zero-extended width and does not introduce the old sign-filled high bits.

### Step 3: Add Focused Coverage

Goal: make the repaired MOVI or integer materialization owner observable in
local backend tests.

Primary target: focused AArch64 backend runtime or dump coverage matching the
classified owner.

Actions:

- Add or extend focused coverage that fails without the repair and passes with
  it.
- Include guardrails for the adjacent repaired variadic HFA/byval path when
  the proof scope can do so cheaply.
- Keep the test semantic; do not assert only one representative output string
  when a lower-level owner can be observed directly.

Completion check:

- Focused coverage proves the repaired owner and adjacent guardrails.

### Step 4: Validate Representative And Classify Residuals

Goal: prove the repair on focused guardrails and the `00204.c`
representative, then hand off any new first bad fact.

Primary target: supervisor-selected focused proof scope including
`c_testsuite_aarch64_backend_src_00204_c`.

Actions:

- Run the delegated focused proof command and record results in `todo.md`.
- Confirm the MOVI sign-extension mismatch is gone.
- Confirm HFA overflow assignment, scalar-FP symbol-load placement, byval
  aggregate lane publication, stdarg cursor, fixed-formal, local/value-home,
  and frame/formal guardrails remain stable.
- If `00204.c` still fails, classify whether the next first bad fact remains
  in this source idea or belongs to another distinct initiative.

Completion check:

- `todo.md` records fresh proof. The localized MOVI zero-extension fault is
  gone, adjacent guardrails remain stable, and any remaining blocker is
  explicitly localized for the next lifecycle decision.
