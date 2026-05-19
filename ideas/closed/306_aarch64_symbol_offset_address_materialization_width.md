# AArch64 Symbol+Offset Address Materialization Register-Width Legality

Status: Closed
Created: 2026-05-19
Closed: 2026-05-19
Split From: ideas/open/295_backend_regex_failure_family_inventory.md

## Closure Notes

Closed after the focused regression guard for
`^c_testsuite_aarch64_backend_src_(00050|00176|00182)_c$` passed against the
valid pre-fix baseline from detached commit `3d4b11762`: before was 0/3,
after was 1/3, `c_testsuite_aarch64_backend_src_00050_c` resolved, and no new
failing tests appeared.

The source idea's owner is satisfied because the focused generated assembly no
longer emits the old AArch64 legality mode where symbol+offset address
formation uses `adrp wN` or a `wN` memory base. The remaining focused failures
are separate residuals: `00176.c` now reaches a runtime mismatch, and
`00182.c` fails on the unrelated large-immediate assembler form
`mov x0, #1234567`.

## Goal

Repair the AArch64 backend path where symbol+offset address materialization
uses a 32-bit register as an address temporary, then feeds that register to
`ldr` or `str` as a memory base.

## Why This Exists

The post-305 backend-regex inventory found a focused compile-stage assembler
legality bucket in existing generated assembly:

```text
00050.c, 00176.c, 00182.c
```

The shared symptom is not a runtime result mismatch and not a missing
expectation. The generated AArch64 assembly forms a symbol+offset address with
`adrp` into a `wN` register and then uses that same `wN` register as a memory
base:

- `00050.c.s`: `adrp w9, v+8` followed by `ldr w9, [w9]`;
- `00176.c.s`: `adrp w10, array+56` / `ldr w10, [w10]` and `adrp w9, array+60` / `ldr w9, [w9]`;
- `00182.c.s`: `adrp w10, __static_local_print_led_3+120` / `ldr w10, [w10]` and `adrp w9, __static_local_print_led_3+124` / `ldr w9, [w9]`.

AArch64 address formation and memory-base operands require 64-bit address
registers. Progress means the backend preserves address values in `xN`
registers through `adrp`/`:lo12:` formation and memory access emission, rather
than matching the current testcase names or rewriting test contracts.

## In Scope

- AArch64 symbol+offset address materialization for global/static objects and
  static-local aggregate/object references.
- Register-width legality for address temporaries used by `adrp`, optional
  low-12 address formation, and `ldr`/`str` memory bases.
- Focused cases `00050.c`, `00176.c`, and `00182.c`.
- Nearby same-owner symbol+offset/global/static-array probes when the
  supervisor selects them to prove the repair is semantic.
- Generated assembly inspection that proves address temporaries remain 64-bit
  address registers before memory access emission.

## Out Of Scope

- `00189.c` externally binding symbol/PIC relocation behavior, including
  `stdout` and GOT/PIC handling, unless future evidence proves it shares this
  exact address-register-width owner.
- Runtime nonzero, runtime mismatch/crash, timeout, hang, and output-storm
  residuals.
- Call-boundary move forms, scalar `mul` printable-rhs gaps, unprepared
  frame-slot source gaps, semantic `lir_to_bir` admission failures, and other
  machine-printer/frontend residual buckets not tied to symbol+offset
  address-register width.
- Closed owner scopes for fused compare-branch operand forms, scalar immediate
  fallback, scalar-cast forms, memory-store source materialization, scalar
  selected-node operand forms, sign-extension assembler spelling, or
  `00205`-specific residuals.
- Expectation rewrites, allowlist changes, unsupported classification changes,
  timeout policy changes, runner behavior changes, CTest registration changes,
  or proof-log policy changes.
- Filename-specific shortcuts or matching only the exact current emitted
  instruction strings.

## Acceptance Criteria

- `00050.c`, `00176.c`, and `00182.c` no longer fail from the old assembler
  legality mode where symbol+offset address formation uses a `wN` address
  temporary or a `wN` memory base.
- Generated AArch64 assembly shows symbol+offset address values are formed and
  carried in 64-bit address registers before `ldr`/`str`.
- The repair is semantic across the focused symbol+offset/global/static-object
  family and does not special-case individual c-testsuite filenames.
- Fresh build proof and the supervisor-selected focused backend c-testsuite
  subset are recorded.
- Any broader backend-regex proof keeps remaining runtime, mismatch, timeout,
  PIC/GOT, and unrelated compile-stage buckets separate from this owner.

## Reviewer Reject Signals

Reject the route if it:

- special-cases `00050.c`, `00176.c`, or `00182.c` by filename, exact symbol
  spelling, or exact current assembly text instead of fixing address temporary
  width for symbol+offset memory references;
- rewrites expected output, allowlists, unsupported classifications, timeout
  policy, runner behavior, CTest registration, or proof-log policy while
  claiming backend capability progress;
- treats `00189.c` as accepted scope without generated-code evidence that the
  same address-register-width repair owns externally binding GOT/PIC symbol
  references such as `stdout`;
- folds runtime nonzero, runtime mismatch/crash, timeout, output-storm,
  call-boundary, scalar arithmetic, frame-slot, or semantic `lir_to_bir`
  failures into this owner without direct generated-code or diagnostic
  evidence that they share the `wN` symbol+offset address-base failure mode;
- only renames helpers or moves code while generated assembly still emits
  `adrp wN, symbol+offset` or uses `wN` as a memory base for symbol+offset
  loads/stores;
- reopens closed owners 285 through 305 from residual failure counts alone
  without proof that their closure boundary was contradicted;
- claims progress from a single narrow named case while nearby symbol+offset
  address materialization remains unexamined or still emits illegal address
  register widths.
