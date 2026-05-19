# AArch64 Symbol+Offset Address Materialization Register-Width Legality Runbook

Status: Active
Source Idea: ideas/open/306_aarch64_symbol_offset_address_materialization_width.md
Split From: ideas/open/295_backend_regex_failure_family_inventory.md

## Purpose

Execute the focused owner split from umbrella inventory idea 295 for AArch64
symbol+offset address materialization that currently emits illegal 32-bit
address temporaries.

## Goal

Make symbol+offset address formation preserve address values in 64-bit
registers through `adrp`/`:lo12:` formation and `ldr`/`str` memory-base use for
the focused cases `00050.c`, `00176.c`, and `00182.c`.

## Core Rule

Fix the semantic AArch64 address-register-width owner. Do not claim progress
through testcase-name matching, exact emitted-instruction matching, expectation
changes, allowlists, unsupported classifications, timeout policy, runner
behavior, CTest registration, or proof-log policy changes.

## Read First

- Source idea:
  `ideas/open/306_aarch64_symbol_offset_address_materialization_width.md`
- Split source:
  `ideas/open/295_backend_regex_failure_family_inventory.md`
- Split evidence preserved in the new source idea and the post-305 note in
  umbrella idea 295.
- Existing generated assembly:
  - `build/c_testsuite_aarch64_backend/src/00050.c.s`
  - `build/c_testsuite_aarch64_backend/src/00176.c.s`
  - `build/c_testsuite_aarch64_backend/src/00182.c.s`
  - `build/c_testsuite_aarch64_backend/src/00189.c.s` only as out-of-scope
    PIC/GOT contrast evidence.

## Current Targets

- `00050.c`: symbol+offset address reference for `v+8`.
- `00176.c`: global scalar-array symbol+offset loads around `array+56` and
  `array+60`.
- `00182.c`: static-local aggregate/object symbol+offset loads around
  `__static_local_print_led_3+120` and `+124`.

## Non-Goals

- Do not include `00189.c` unless new generated-code evidence proves the same
  repair owns externally binding GOT/PIC symbol references such as `stdout`.
- Do not address runtime nonzero, runtime mismatch/crash, timeout, hang, or
  output-storm residuals in this owner.
- Do not fold call-boundary moves, scalar arithmetic printer gaps,
  unprepared frame-slot sources, semantic `lir_to_bir` admission failures, or
  unrelated machine-printer/frontend buckets into this runbook.
- Do not reopen closed owners 285 through 305 from pass counts alone.
- Do not edit expectations, allowlists, unsupported classifications, timeout
  policy, runner behavior, CTest registration, or proof-log policy.

## Working Model

The failing cases already reach AArch64 assembly emission. The shared illegal
form is address materialization into a `wN` register followed by memory access
that uses that same `wN` as the base. The likely repair surface is the backend
path that chooses or preserves register width for symbol+offset address values,
not the c-testsuite registration or runtime harness.

## Execution Rules

- Inspect the generated assembly and lowering/printing path before editing.
- Repair the address-value width decision at the semantic lowering,
  materialization, selection, or printer-admission point that owns symbol+offset
  memory references.
- Keep data values and address values distinct: only address temporaries and
  memory bases require 64-bit address registers.
- Prove the focused cases first, then broaden only enough to guard nearby
  symbol+offset/global/static-object forms.
- Record all packet progress and proof in `todo.md`.

## Steps

### Step 1: Locate The Symbol+Offset Address Width Owner

Goal: identify where `wN` is selected or preserved for symbol+offset address
temporaries.

Primary target: AArch64 lowering/materialization/selection/printer path for
symbol+offset memory operands

Actions:

- Inspect generated assembly for `00050.c`, `00176.c`, and `00182.c` and
  trace the relevant symbol+offset memory references back through the backend.
- Identify whether the bad width is introduced during semantic lowering,
  address materialization, register allocation/selection, or final printing.
- Confirm that `00189.c` is a PIC/GOT relocation contrast case, not proof for
  this owner.

Completion check:

- `todo.md` records the concrete owner function/path and why it covers all
  three focused cases without absorbing unrelated buckets.

### Step 2: Repair Address Register Width For Symbol+Offset Memory References

Goal: emit legal 64-bit address temporaries and memory bases for the focused
symbol+offset family.

Primary target: the owner identified in Step 1

Actions:

- Update the semantic owner so symbol+offset address values are represented,
  materialized, selected, and printed as 64-bit address registers.
- Preserve existing 32-bit data-result behavior for loaded/stored scalar values
  where the data width is 32-bit.
- Avoid filename, symbol-name, or exact-instruction shortcuts.
- Do not change test expectations, runner behavior, CTest registration, or
  unsupported classifications.

Completion check:

- Fresh build/compile proof succeeds and generated assembly no longer contains
  illegal `wN` address temporaries or `wN` memory bases for the focused
  symbol+offset references.

### Step 3: Focused Proof And Nearby Same-Owner Guard

Goal: prove the repair on the focused family and check nearby same-owner forms.

Primary target: supervisor-selected focused backend c-testsuite subset

Actions:

- Rerun the focused backend c-tests for `00050.c`, `00176.c`, and `00182.c`.
- Inspect generated assembly for 64-bit address registers through
  symbol+offset memory access.
- Add nearby symbol+offset/global/static-object cases only when they exercise
  the same owner and do not turn the packet into broad inventory work.

Completion check:

- `todo.md` records focused proof results and any remaining residuals,
  explicitly separating out-of-scope runtime, mismatch, timeout, and PIC/GOT
  buckets.

### Step 4: Broader Backend-Regex Acceptance Check

Goal: ensure the focused repair is non-regressive at the backend-regex scope
selected by the supervisor.

Primary target: main-build backend-regex proof

Actions:

- Run the supervisor-selected broader backend-regex command after focused proof
  is green.
- Compare remaining failures against the source idea boundary.
- Do not claim closure from pass-count movement alone; explain semantic bucket
  movement and residual buckets.

Completion check:

- Canonical proof is recorded in `test_after.log` or the delegated proof
  artifact, and `todo.md` is ready for supervisor review of whether idea 306 can
  close or needs another focused packet.
