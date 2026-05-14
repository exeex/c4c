# AArch64 C-Testsuite Backend Full Scan

Status: Open
Created: 2026-05-14
Parent Context: ideas/open/229_aarch64_codegen_markdown_shards_to_cpp.md

## Intent

Establish an AArch64 backend version of the vendored
`tests/c/external/c-testsuite/src` test corpus, run the corpus once to discover
the real failure surface, and classify the results without turning the scan
into a broad repair route.

The first milestone is not to make every case pass. The first milestone is a
truthful AArch64 backend scan: register the cases, run them through the
AArch64 backend route, classify failures, and preserve actionable follow-up
ideas or drafts for later review.

## Ordering Guard

Run this idea only after idea 224 completes, or after 224 is explicitly paused
or accepted far enough that shared MIR printer migration noise will not pollute
the c-testsuite failure inventory.

If 224 is still actively moving printer boundaries, do not start the full scan.
The scan should inventory AArch64 backend and prepared-BIR failures, not
transient failures from an unfinished printer migration.

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

## Scope

This is a discovery, registration, scan, and failure-classification idea.
Allowed changes are limited to:

- runner or registration wiring needed to execute the intended AArch64 backend
  c-testsuite route
- trivial build or path fixes that make the scan target honest
- easy low-risk backend fixes that are already covered by existing prepared
  BIR facts and do not broaden the route
- failure inventory preservation and follow-up idea or draft creation

Deeper AArch64 lowering repairs, shared MIR repairs, BIR semantic changes, or
prepared-BIR mechanism work must become follow-up ideas or drafts instead of
being absorbed into this scan.

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
while doing the scan. If the fix needs deeper AArch64 lowering design, shared
MIR behavior, or new BIR/prepared-BIR facts, stop at classification and create
a follow-up idea or draft.

## Follow-Up Rule

If a failure requires new BIR/prepared-BIR facts, a new prepared-module
mechanism, a change in shared MIR semantics, or a nontrivial AArch64 lowering
repair, do not patch around it inside the scan.

Instead, create a follow-up idea or draft describing the missing mechanism and
the failing c-testsuite evidence. Use a filename that makes the gap easy to
review, for example:

```text
ideas/draft/NN_bir_<failure_family>_from_aarch64_c_testsuite.md
ideas/open/NN_aarch64_<failure_family>_from_c_testsuite.md
```

Each follow-up should include:

- representative failing c-testsuite cases
- observed failure mode and command
- why this is not just runner or registration noise
- proposed owner layer: AArch64 lowering, shared MIR, BIR, or prepared BIR
- expected AArch64 backend consumer once the gap is fixed
- proof subset to rerun after the follow-up is repaired

These follow-ups are review material. Do not activate deeper repair work during
this scan unless explicitly requested after review.

## Boundaries

- Do not start while active 224 printer migration noise would contaminate the
  inventory.
- Do not convert broad c-testsuite failures into unsupported expectations.
- Do not shrink the corpus merely to make the AArch64 backend label green.
- Do not add testcase-shaped AArch64 lowering shortcuts for individual
  c-testsuite files.
- Do not put missing BIR or shared MIR semantics into target codegen as hidden
  state.
- Do not activate deeper follow-up ideas during this scan unless explicitly
  requested after review.
- Do not claim scan completion without preserving the failure inventory.

## Completion Signal

This idea is complete when the AArch64 backend c-testsuite scan is registered
and runnable after the 224 ordering guard is satisfied, the corpus has been run
once through that route, easy nonsemantic or low-risk fixes have either landed
or been explicitly deferred, and every remaining deeper AArch64, shared MIR,
BIR, or prepared-BIR failure family has a corresponding reviewable follow-up
idea or draft.

## Reviewer Reject Signals

Reject the route if it starts while 224 printer migration noise is still likely
to distort the inventory, if the "AArch64 backend" scan actually exercises host
backend or LLVM IR fallback, if failures are hidden by weakening the corpus, if
runtime-unavailable cases are counted as backend passes, if AArch64 codegen
grows semantic facts that should have been prepared by BIR or shared MIR, if
deep lowering repairs are claimed as part of the scan instead of split out, or
if follow-up ideas lack concrete failing case evidence.
