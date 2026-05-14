# AArch64 Branch Label Materialization From C-Testsuite Scan

Status: Draft
Created: 2026-05-14
Parent Context: ideas/open/230_aarch64_c_testsuite_backend_full_scan.md

## Intent

Repair the AArch64 backend label emission gap exposed by c-testsuite cases
that produce assembly referencing undefined temporary `.LBB*` symbols.

## Evidence From The Scan

Observed scan command:

```text
ctest --test-dir build-aarch64-scan --output-on-failure -R '^c_testsuite_aarch64_backend_'
```

Bucket count:

- Part of 43 `[BACKEND_FAIL]` assembly/link toolchain failures.

Representative undefined-label cases:

- `src/00005.c`
- `src/00006.c`
- `src/00007.c`
- `src/00156.c`
- `src/00200.c`

Observed failure mode:

- clang reports undefined temporary `.LBB*` symbols from generated AArch64
  assembly.

## Owner Layer

AArch64 selected machine control-flow lowering and terminal machine printer.

The owner should preserve structured block/branch identity through selected
machine records and print definitions and references from the same label
authority.

## Scope

- Identify where AArch64 machine lowering records branch targets and basic
  block labels.
- Ensure every printed branch target has a matching printed label definition.
- Preserve stable label identity through machine-node selection and terminal
  printing.
- Validate with representative c-testsuite cases that currently produce
  undefined `.LBB*` references.

## Out Of Scope

- Do not patch individual testcase names or rendered `.LBB*` strings.
- Do not infer control-flow structure from already printed assembly.
- Do not weaken assembly/link expectations.
- Do not combine this route with scalar ALU operand repair, runtime runner
  wiring, or unrelated call/frame work.

## Expected Backend Consumer

The AArch64 backend scan should consume this by moving affected cases from
`[BACKEND_FAIL]` undefined-label assembly rejection to either linked binaries,
runtime-unavailable, runtime mismatch, or a more precise remaining backend
failure.

## Proof Subset To Rerun

Start with:

```text
ctest --test-dir build-aarch64-scan --output-on-failure -R 'c_testsuite_aarch64_backend_src_0000[567]_c|c_testsuite_aarch64_backend_src_00156_c|c_testsuite_aarch64_backend_src_00200_c'
```

Then rerun the full AArch64 backend c-testsuite route to count remaining
`[BACKEND_FAIL]` cases and confirm no new undefined-label family appears.

## Acceptance Criteria

- Representative cases no longer contain branch references to undefined
  temporary labels.
- Label definitions and branch references come from structured machine/control
  facts, not ad hoc string matching.
- Remaining failures, if any, are classified under a different concrete owner
  layer.

## Reviewer Reject Signals

Reject the route if it hard-codes `.LBB*` names for the representative cases,
adds testcase-shaped label emission shortcuts, accepts assembly/link failures
by weakening tests, or leaves branch references and label definitions sourced
from unrelated text-generation paths.
