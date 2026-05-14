# AArch64 C-Testsuite Backend Full Scan

Status: Open
Created: 2026-05-14
Parent Context: ideas/open/229_aarch64_codegen_markdown_shards_to_cpp.md

## Intent

Establish an AArch64 backend version of the vendored
`tests/c/external/c-testsuite/src` test corpus, run the corpus once to discover
the real failure surface, fix only easy backend issues immediately, and turn
deeper prepared-BIR gaps into draft ideas for human review.

The first milestone is not to make every case pass. The first milestone is a
truthful AArch64 backend scan: register the cases, run them through the
AArch64 backend route, classify failures, and preserve actionable follow-up
plans under `ideas/draft/` for tomorrow's review.

## Current Context

`tests/c/external/c-testsuite/src` already exists and contains the vendored
case sources plus `.expected`, `.tags`, and `.otags` sidecars. Current CMake
registration for c-testsuite lives in `tests/c/external/CMakeLists.txt` and
uses `tests/c/external/c-testsuite/RunCase.cmake`.

Backend c-testsuite registration currently depends on
`ENABLE_C_TESTSUITE_BACKEND_TESTS`, and the backend target selection is tied to
host CPU. The AArch64 path should be made explicit enough that the full scan
can intentionally exercise `--codegen asm --target aarch64-unknown-linux-gnu`
instead of accidentally testing only the host backend.

## Desired Test Shape

Create or adjust test registration so there is a clear AArch64 backend
c-testsuite scan over the vendored `src` corpus:

- source corpus: `tests/c/external/c-testsuite/src/*.c`
- expected output: existing `*.c.expected` sidecars
- frontend/compiler: `c4cll`
- backend route: `--codegen asm --target aarch64-unknown-linux-gnu`
- assembly check: produced output must be AArch64 backend assembly, not
  fallback LLVM IR
- object/link step: use `clang --target=aarch64-unknown-linux-gnu` or the
  repo's existing AArch64 toolchain path
- runtime step: run directly on AArch64 hosts, or through an explicit supported
  runner/emulator only if that runner is added intentionally

If the local environment cannot execute AArch64 binaries, still preserve the
compile/assemble/link failure inventory and clearly separate runtime-unavailable
cases from compiler/backend failures. Do not hide this distinction by marking
cases as pass.

## First Full Scan

Run the AArch64 backend version across the corpus once and save the result in a
stable artifact or summary that can be reviewed. The scan should answer:

- how many cases registered
- how many compile to AArch64 assembly
- how many assemble/link
- how many run and match expected output
- which cases fail, grouped by failure class

Failure classes should distinguish at least:

- frontend parse/sema failure
- BIR or prepared-BIR construction failure
- AArch64 backend lowering failure
- fallback/non-AArch64 output
- assembler/linker/toolchain failure
- runtime mismatch
- runtime unavailable on this host

## Easy Fix Rule

If a failing case has an obvious, low-risk fix that does not change source
intent, repair it in the same execution route and rerun the relevant subset.
Examples of acceptable easy fixes:

- missing test registration or path wiring
- obvious runner bug in the new AArch64 scan path
- trivial AArch64 backend bug already covered by prepared BIR facts
- clear build-system mismatch that prevents the intended route from running

Do not broaden the active implementation task into a large semantic repair
while doing the scan.

## BIR Gap Rule

If a failure requires new BIR/prepared-BIR facts, a new prepared-module
mechanism, or a change in what BIR records for lowering, do not patch around it
inside AArch64 backend code.

Instead, create a draft idea under `ideas/draft/` describing the missing BIR
mechanism and the failing c-testsuite evidence. Use a filename that makes the
gap easy to review, for example:

```text
ideas/draft/NN_bir_<failure_family>_from_aarch64_c_testsuite.md
```

Each BIR-gap draft should include:

- representative failing c-testsuite cases
- observed failure mode and command
- why this is a BIR/prepared-BIR gap rather than an AArch64-only bug
- proposed BIR data or mechanism to add
- expected AArch64 backend consumer once BIR is fixed
- proof subset to rerun after the BIR gap is repaired

These drafts are review material. Leave them in `ideas/draft/` for the human
review pass before activating repair work.

## Boundaries

- Do not convert broad c-testsuite failures into unsupported expectations.
- Do not shrink the corpus merely to make the AArch64 backend label green.
- Do not add testcase-shaped AArch64 lowering shortcuts for individual
  c-testsuite files.
- Do not put missing BIR semantics into target codegen as hidden state.
- Do not activate BIR-gap draft ideas during this scan unless explicitly
  requested after review.
- Do not claim scan completion without preserving the failure inventory.

## Completion Signal

This idea is complete when the AArch64 backend c-testsuite scan is registered
and runnable, the corpus has been run once through that route, easy nonsemantic
or low-risk fixes have either landed or been explicitly deferred, and every
remaining failure that appears to require BIR/prepared-BIR work has a
corresponding reviewable draft idea under `ideas/draft/`.

## Reviewer Reject Signals

Reject the route if the "AArch64 backend" scan actually exercises host backend
or LLVM IR fallback, if failures are hidden by weakening the corpus, if
runtime-unavailable cases are counted as backend passes, if AArch64 codegen
grows semantic facts that should have been prepared by BIR, or if BIR-gap
drafts lack concrete failing case evidence.
