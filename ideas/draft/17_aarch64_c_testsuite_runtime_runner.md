# AArch64 C-Testsuite Runtime Runner From Backend Scan

Status: Draft
Created: 2026-05-14
Parent Context: ideas/open/230_aarch64_c_testsuite_backend_full_scan.md

## Intent

Provide an explicit runtime execution route for AArch64 c-testsuite backend
cases that already compile, assemble, and link on non-AArch64 hosts.

The scan currently has 149 cases tagged `[RUNTIME_UNAVAILABLE]`. These are not
backend passes. They produced AArch64 assembly and linked binaries, then
stopped before expected-output comparison because no
`C_TESTSUITE_AARCH64_BACKEND_RUNNER` was configured.

## Evidence From The Scan

Observed scan command:

```text
ctest --test-dir build-aarch64-scan --output-on-failure -R '^c_testsuite_aarch64_backend_'
```

Observed count:

- 149 runtime-unavailable cases
- 149 linked AArch64 binaries under
  `build-aarch64-scan/c_testsuite_aarch64_backend`

Representative cases:

- `c_testsuite_aarch64_backend_src_00001_c`
- `c_testsuite_aarch64_backend_src_00002_c`
- `c_testsuite_aarch64_backend_src_00003_c`
- `c_testsuite_aarch64_backend_src_00123_c`

Observed failure class:

- CTest-visible tag: `[RUNTIME_UNAVAILABLE]`
- The cases stop honestly before runtime comparison because there is no native
  AArch64 host execution route or configured supported runner/emulator.

## Owner Layer

Test runner and environment wiring for the AArch64 backend c-testsuite route.

This is not an AArch64 lowering, BIR, prepared-BIR, or machine-printer repair.

## Scope

- Define the supported contract for `C_TESTSUITE_AARCH64_BACKEND_RUNNER`.
- Add or document the expected runner invocation shape for linked AArch64
  binaries.
- Preserve the distinction between runtime-unavailable, runtime mismatch, and
  backend compile/link failures.
- Rerun the already-linked representative cases through the configured runner
  when available.

## Out Of Scope

- Do not count runtime-unavailable cases as passes.
- Do not weaken expected outputs or c-testsuite sidecars.
- Do not add backend codegen repairs as part of runner enablement.
- Do not require every development host to have an AArch64 runtime; the route
  may remain conditional on an explicit runner.

## Expected Backend Consumer

The AArch64 backend scan consumes this route after assembly and link succeed,
so it can classify cases as runtime pass, runtime mismatch, runtime crash, or
runtime unavailable.

## Proof Subset To Rerun

Use representative runtime-unavailable cases first:

```text
ctest --test-dir build-aarch64-scan --output-on-failure -R 'c_testsuite_aarch64_backend_src_0000[123]_c|c_testsuite_aarch64_backend_src_00123_c'
```

Then rerun the full AArch64 backend c-testsuite route with an explicit
`C_TESTSUITE_AARCH64_BACKEND_RUNNER` configured.

## Acceptance Criteria

- Runtime-unavailable cases remain explicitly tagged when no runner is present.
- With a valid runner configured, representative linked binaries execute and
  compare against their `.expected` sidecars.
- Runtime mismatches are reported separately from backend and toolchain
  failures.

## Reviewer Reject Signals

Reject the route if it reclassifies runtime-unavailable cases as passes, hides
missing runner configuration behind weaker expectations, requires backend
codegen changes to prove runner wiring, or collapses runtime mismatch and
runtime-unavailable diagnostics into the same bucket.
