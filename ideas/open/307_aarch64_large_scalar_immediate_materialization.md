# AArch64 Large Scalar Immediate Materialization

Status: Open
Created: 2026-05-19
Split From: ideas/open/295_backend_regex_failure_family_inventory.md

## Goal

Repair the AArch64 backend path where a scalar integer constant reaches final
assembly printing as an illegal single-instruction `mov` immediate instead of
being materialized through a valid AArch64 constant sequence.

## Why This Exists

The post-306 backend-regex inventory selected 352 tests, passed 306, and left
46 failures, all under `c_testsuite_aarch64_backend_*`. The read-only
classification found one crisp assembler-stage owner in
`c_testsuite_aarch64_backend_src_00182_c`: generated assembly contains
`mov x0, #1234567`, which the assembler rejects because that large immediate
is not encodable as the emitted AArch64 move form.

This is a focused semantic backend capability, not a filename target:
selected scalar constants that cannot be represented by the current printable
move/immediate form must be lowered or materialized into legal AArch64
instruction sequences before assembly.

## In Scope

- AArch64 scalar integer constant materialization for large immediates that
  currently reach printing as illegal `mov xN, #imm` forms.
- The focused failing case `00182.c`, specifically the post-306 assembler
  residual `mov x0, #1234567`.
- Nearby generated-assembly probes for scalar constant materialization when
  the supervisor selects them to prove the repair is semantic.
- Build and focused backend c-testsuite proof that the old illegal large
  immediate assembler failure is gone.

## Out Of Scope

- `00189.c` externally binding symbol/PIC relocation behavior, including
  non-PIC relocation against `stdout`.
- Runtime nonzero, runtime mismatch, runtime crash, timeout, and output-storm
  buckets from the post-306 inventory.
- Symbol+offset address-register-width legality already closed by idea 306.
- Scalar add/sub/bitwise instruction-immediate fallback already closed by idea
  299 unless generated-code evidence proves a shared constant materialization
  primitive needs to be reused.
- Call-boundary move gaps, scalar `mul` printable-rhs gaps, unprepared
  frame-slot source gaps, and semantic `lir_to_bir` admission residuals.
- Expectation rewrites, allowlist changes, unsupported classification changes,
  timeout policy changes, runner behavior changes, CTest registration changes,
  or proof-log policy changes.
- Filename-specific or exact-instruction-string shortcuts.

## Acceptance Criteria

- `c_testsuite_aarch64_backend_src_00182_c` no longer fails from the old
  assembler rejection of `mov x0, #1234567`.
- Generated AArch64 assembly materializes large scalar immediates through legal
  AArch64 forms rather than emitting an unencodable single `mov` immediate.
- The repair is expressed as backend constant-materialization behavior and
  does not special-case `00182.c`, the literal `1234567`, or one emitted line.
- Fresh build proof and the supervisor-selected focused backend c-testsuite
  subset are recorded.
- Any broader backend-regex proof reports remaining PIC/global relocation and
  runtime buckets separately from this owner.

## Reviewer Reject Signals

Reject the route if it:

- special-cases `00182.c`, the literal `1234567`, or the exact text
  `mov x0, #1234567` instead of repairing large scalar immediate
  materialization for AArch64;
- rewrites expected output, allowlists, unsupported classifications, timeout
  policy, runner behavior, CTest registration, or proof-log policy while
  claiming backend capability progress;
- treats `00189.c` PIC/global-symbol relocation as accepted scope without
  generated-code evidence that the same constant materialization rule owns
  externally binding symbol relocation behavior;
- folds runtime nonzero, mismatch, crash, timeout, output-storm,
  call-boundary, scalar `mul`, frame-slot, or semantic `lir_to_bir` failures
  into this owner without direct generated-code or diagnostic evidence that
  they share the large scalar immediate materialization failure mode;
- only renames helpers or broadens the printer while generated assembly still
  emits illegal large scalar immediates;
- reopens closed owners 285 through 306 from residual failure counts alone
  without proof that their closure boundary was contradicted;
- claims progress from a single narrow named case while nearby large scalar
  immediate materialization remains unexamined or still emits illegal AArch64
  immediate forms.
