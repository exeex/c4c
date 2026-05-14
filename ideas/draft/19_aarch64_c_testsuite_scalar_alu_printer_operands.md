# AArch64 Scalar ALU Printer Operands From C-Testsuite Scan

Status: Draft
Created: 2026-05-14
Parent Context: ideas/open/230_aarch64_c_testsuite_backend_full_scan.md

## Intent

Repair the AArch64 scalar integer ALU lowering and printer operand gaps exposed
by c-testsuite cases that either cannot be printed by the machine printer or
assemble into invalid AArch64 operands.

This is a structured scalar ALU capability route. It must not become a set of
case-name or text-template patches.

## Evidence From The Scan

Observed scan command:

```text
ctest --test-dir build-aarch64-scan --output-on-failure -R '^c_testsuite_aarch64_backend_'
```

Related bucket counts:

- Part of 43 `[BACKEND_FAIL]` assembly/link toolchain failures.
- Part of 28 `[FRONTEND_FAIL]` AArch64 lowering / machine-printer failures.

Representative invalid-assembly cases:

- `src/00012.c`: invalid operand `sub w0, x19, #8`
- `src/00018.c`: invalid instruction operands
- `src/00021.c`: invalid instruction operands
- `src/00124.c`: expected compatible register/symbol/integer range diagnostic

Representative machine-printer cases:

- `src/00024.c`: cannot print scalar `sub` with immediate lhs/register rhs
- `src/00027.c`: cannot print scalar `or`
- `src/00028.c`: cannot print scalar `and`
- `src/00104.c`: cannot print scalar `xor`
- `src/00213.c`: cannot print scalar `add` immediate outside `0..4095`

## Owner Layer

AArch64 scalar integer operation lowering, selected machine-node records, and
terminal printer operand validation.

Prepared/register layers should still own value homes and register class
authority. AArch64 codegen should consume those facts and choose valid
instruction forms or explicit fallback sequences.

## Scope

- Add or complete structured selected-machine coverage for scalar `add`,
  `sub`, `and`, `or`, and `xor` operand forms seen in the scan.
- Validate GPR width compatibility before printing instructions.
- Lower non-encodable immediates, reversed immediate/register forms, or other
  unsupported instruction shapes through structured legal sequences instead of
  emitting invalid text.
- Keep diagnostics explicit when an operand form is outside current capability.

## Out Of Scope

- Do not rebuild the archived accumulator-style integer operation surface.
- Do not infer operand legality from rendered register names alone.
- Do not special-case the listed c-testsuite filenames.
- Do not merge this route with float/cast, i128, call/frame, or memory machine
  node work unless the active source idea is explicitly switched.
- Do not weaken backend expectations or mark failures unsupported as progress.

## Expected Backend Consumer

The AArch64 backend scan should consume this route by turning representative
scalar ALU printer failures and invalid-assembly failures into valid AArch64
assembly that either links or reaches a more precise later failure bucket.

## Proof Subset To Rerun

Start with both printer and assembler representatives:

```text
ctest --test-dir build-aarch64-scan --output-on-failure -R 'c_testsuite_aarch64_backend_src_00012_c|c_testsuite_aarch64_backend_src_00018_c|c_testsuite_aarch64_backend_src_00021_c|c_testsuite_aarch64_backend_src_00024_c|c_testsuite_aarch64_backend_src_00027_c|c_testsuite_aarch64_backend_src_00028_c|c_testsuite_aarch64_backend_src_00104_c|c_testsuite_aarch64_backend_src_00124_c|c_testsuite_aarch64_backend_src_00213_c'
```

Then rerun the full AArch64 backend c-testsuite route to check the
`[BACKEND_FAIL]` and `[FRONTEND_FAIL]` bucket counts.

## Acceptance Criteria

- Representative scalar ALU cases no longer fail due to unprintable structured
  nodes or invalid AArch64 operands.
- Width, immediate-range, and operand-order rules are represented as structured
  lowering/printer facts.
- Unsupported forms fail with explicit diagnostics rather than invalid
  assembler text.

## Reviewer Reject Signals

Reject the route if it matches the representative filenames, emits invalid
assembly and relies on weakened checks, treats expectation-only changes as
capability progress, hides width mismatches behind string rewrites, or
recreates an accumulator/scratch-register convention instead of consuming
prepared value-home facts.
