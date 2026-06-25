# c-testsuite RV64 asm/objdump Roundtrip Scan Target

## Closure

Closed after adding the opt-in
`rv64_c_testsuite_asm_roundtrip_scan` build target, direct CMake-script runner,
explicit initial allowlist, unsupported-family note, and manual scan
documentation. The accepted close proof used the opt-in scan target with both
allowlisted cases passing and a CTest membership query confirming the scan is
not registered as a default CTest test.

## Goal

Add a large-scale, opt-in test target for the `c4c-as` and `c4c-objdump`
toolchain using the vendored c-testsuite C sources as generated assembly input.

This is not a normal full-CTest gate. It is a broader scan target that can be
run manually, in nightly CI, or when validating assembler/disassembler
coverage.

## Motivation

The focused RV64 roundtrip contract proves a curated RV64I plus c4c EV64
corpus. That is useful, but it does not exercise the wide variety of `.s`
patterns produced by `c4cll --codegen asm` from real C programs.

We need a larger route:

```text
c-testsuite/*.c
  -> c4cll --codegen asm --target riscv64-linux-gnu
  -> c4c-as
  -> c4c-objdump
  -> c4c-as
  -> compare stable object/text contract
```

This should find missing assembler/parser/disassembler coverage without making
ordinary `ctest --test-dir build -j --output-on-failure` too slow or too noisy.

## Scope

Create an opt-in test workflow for many c-testsuite cases.

Suggested interface:

- a CMake custom target such as `rv64_c_testsuite_asm_roundtrip_scan`, or
- an option-gated CTest group disabled by default, or
- a script under `scripts/` with a documented CMake target wrapper.

The default full test suite must not run the large scan unless explicitly
enabled.

## Required Route

For each selected c-testsuite `.c` file:

```text
case.c
  -> c4cll --codegen asm --target riscv64-linux-gnu -> case.pass0.s
  -> c4c-as case.pass0.s -> case.pass1.o
  -> c4c-objdump case.pass1.o -> case.pass1.s
  -> c4c-as case.pass1.s -> case.pass2.o
  -> c4c-objdump case.pass2.o -> case.pass2.s
  -> c4c-as case.pass2.s -> case.pass3.o
```

The stable comparison should check:

- `case.pass1.s == case.pass2.s`
- `case.pass2.o == case.pass3.o`

If a case fails because generated `.s` uses an unsupported assembler feature,
the scan should report that case and failure family clearly. It must not silently
skip or rewrite unsupported lines.

## In Scope

- Selecting an initial c-testsuite allowlist for the scan.
- A runner that records per-case pass/fail/unsupported diagnostics.
- Clear summary output with failing case paths.
- Stable output directory under the build tree.
- Timeout handling per case.
- A documented command to run the scan.
- Optional CI/nightly integration point that is separate from normal full CTest.

## Out Of Scope

- Requiring this scan to pass as part of ordinary full CTest.
- Using external `as` or `objdump` as the source of truth.
- Runtime execution of the generated binaries.
- Broad c-testsuite semantic correctness; this scan is about assembler,
  disassembler, and object roundtrip stability.
- Silently accepting unsupported generated assembly.

## Acceptance Criteria

- There is an explicit command or build target to run the large RV64 asm
  roundtrip scan.
- Ordinary full `ctest --test-dir build -j --output-on-failure` does not run
  the large scan by default.
- The scan takes c-testsuite `.c` inputs through `c4cll --codegen asm`,
  `c4c-as`, and `c4c-objdump`.
- The runner checks both canonical text stability and object stability after
  two passes.
- Failures report the case file and stage (`c4cll-asm`, `c4c-as-pass1`,
  `objdump-pass1`, `c4c-as-pass2`, `objdump-pass2`, `c4c-as-pass3`, or
  comparison).
- The initial allowlist and known unsupported families are explicit.

## Reviewer Reject Signals

- The scan is registered as an always-on full CTest test.
- The runner treats unsupported generated assembly as a silent pass.
- The route bypasses `c4c-as` or `c4c-objdump`.
- The proof only uses the curated hand-written RV64 corpus from idea 352.
- The scan compares only text or only object bytes, but not both.
