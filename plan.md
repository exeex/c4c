# AArch64 OPI Pointer Integer Operation Result Publication Runbook

Status: Active
Source Idea: ideas/open/333_aarch64_opi_pointer_integer_operation_result_publication.md
Split from: ideas/open/332_aarch64_movi_zero_extension_materialization.md

## Purpose

Continue the AArch64 `00204.c` representative after commit `9ad547218`
repaired MOVI zero-extension. The new first bad fact is in `opi()`, not in
MOVI immediate materialization.

## Goal

Classify and repair the generated-code owner that loses the result of a simple
pointer or integer operation before `pll` observes it.

## Core Rule

Repair a general AArch64 scalar result, return, or call-output publication
capability. Do not special-case `00204.c`, `opi`, `addip0`, one literal, one
register, one stack slot, one source line, or one emitted instruction
sequence.

## Read First

- `ideas/open/333_aarch64_opi_pointer_integer_operation_result_publication.md`
- `todo.md`
- generated AArch64 artifacts for `00204.c` under
  `build/c_testsuite_aarch64_backend/src/`
- existing focused `test_after.log`, when present

## Current Targets

- Representative external case:
  - `c_testsuite_aarch64_backend_src_00204_c`
- Current first bad fact:
  - source: `tests/c/external/c-testsuite/src/00204.c:476`
  - expression: `pll(addip0(x))` with `x = 1000`
  - expected: `3e8`
  - actual: `c220ecc6`
- Current generated evidence:
  - `addip0` emits `add w0, w0, #0` followed by `mov x0, x13`
  - the caller saves returned `x0` to `x20` but later publishes `x19` to `pll`

## Non-Goals

- Do not reopen MOVI zero-extension or immediate cast folding unless fresh
  generated-code evidence shows a remaining MOVI mismatch.
- Do not reopen HFA/floating, byval aggregate, stdarg cursor, aggregate
  `va_arg`, fixed-formal, local/value-home, frame/formal, or large-frame
  repairs without direct generated-code evidence.
- Do not change semantic admission, runners, timeout policy, expectations,
  unsupported classifications, proof-log policy, or CTest registration.

## Working Model

The representative now reaches `opi()` after the repaired variadic and MOVI
sections. The first `opi()` call should transport the value `1000` through
`addip0` and into `pll`, but generated code appears to compute or return one
value and publish a different register to the output call. The next packet
should identify the first owner where the computed scalar result stops being
the value returned or observed.

## Execution Rules

- Keep routine packet progress in `todo.md`.
- Start from generated-code evidence for the `opi()` first bad fact before
  editing implementation.
- Preserve the repaired MOVI zero-extension, HFA overflow assignment,
  scalar-FP symbol-load placement, byval aggregate lane publication, stdarg
  cursor, fixed-formal, and local/value-home guardrails.
- Prefer focused backend coverage for the localized scalar result, return, or
  call-output owner before relying on the external c-testsuite representative.
- If `00204.c` advances past this residual, record the next first bad fact in
  `todo.md` and return it to lifecycle classification if it belongs to another
  owner.

## Ordered Steps

### Step 1: Localize The OPI Result Publication Owner

Goal: identify the first generated-code owner that loses the expected
`addip0(x)` value before `pll` observes it.

Primary targets:

- generated BIR, prepared records, MIR, and AArch64 assembly for `00204.c`
- AArch64 scalar ALU result, return value, call-result capture, and call-output
  operand publication paths implicated by the generated evidence

Actions:

- Map `pll(addip0(x))` from source expression through BIR, prepared records,
  generated function `addip0`, caller result capture, and the observing `pll`
  call.
- Determine whether the first divergence is scalar ALU result home
  publication, no-op arithmetic lowering, return register selection,
  call-result capture, call-output operand publication, or another concrete
  generated-code owner.
- Record the owner, source value, prepared type or width, result home, return
  register, caller-published operand, and emitted instruction sequence in
  `todo.md`.

Completion check:

- `todo.md` names the first owner with generated-code evidence and separates
  it from MOVI, HFA/floating, byval, stdarg cursor, fixed-formal, local/value,
  and frame/formal owners.

### Step 2: Repair The Classified Owner

Goal: repair the localized AArch64 scalar result, return, or call-output
publication path generally.

Primary target: the backend lowering files identified by Step 1.

Actions:

- Apply the narrow semantic repair for the classified owner.
- Preserve existing guardrails for MOVI zero-extension, HFA overflow
  assignment, scalar-FP symbol-load placement, byval aggregate lane
  publication, aggregate `va_arg`, `va_start`, fixed-formal entry
  publication, local/value-home publication, and frame/formal publication.
- Avoid testcase-shaped matching on the representative source, `opi` block,
  value literals, registers, stack offsets, or printed assembly.

Completion check:

- Generated code returns and publishes the `addip0(x)` result expected by the
  source semantics and does not preserve the old stale-register output shape.

### Step 3: Add Focused Coverage

Goal: make the repaired scalar result, return, or call-output owner observable
in local backend tests.

Primary target: focused AArch64 backend runtime or dump coverage matching the
classified owner.

Actions:

- Add or extend focused coverage that fails without the repair and passes with
  it.
- Include guardrails for the adjacent repaired MOVI and variadic/byval paths
  when the proof scope can do so cheaply.
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
- Confirm the `addip0(x)` mismatch is gone.
- Confirm MOVI zero-extension, HFA overflow assignment, scalar-FP symbol-load
  placement, byval aggregate lane publication, stdarg cursor, fixed-formal,
  local/value-home, and frame/formal guardrails remain stable.
- If `00204.c` still fails, classify whether the next first bad fact remains
  in this source idea or belongs to another distinct initiative.

Completion check:

- `todo.md` records fresh proof. The localized OPI result publication fault is
  gone, adjacent guardrails remain stable, and any remaining blocker is
  explicitly localized for the next lifecycle decision.
