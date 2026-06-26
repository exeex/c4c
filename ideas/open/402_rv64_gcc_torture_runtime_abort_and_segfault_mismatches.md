# RV64 gcc_torture Runtime Abort And Segfault Mismatches

Status: Open
Type: Follow-up repair idea
Parent: `ideas/open/354_rv64_gcc_torture_prepared_module_shape_classification.md`

## Goal

Classify and repair the current RV64 gcc_torture cases that compile, link, and
run through qemu but diverge from clang with aborts or segmentation faults.

## Why This Exists

The 2026-06-26 reopened 354 classification found 34 runtime mismatches:

- 23 `clang_exit=0 c4c_exit=Subprocess aborted`
- 11 `clang_exit=0 c4c_exit=Segmentation fault`

Representatives:

- `tests/c/external/gcc_torture/src/20000113-1.c`
- `tests/c/external/gcc_torture/src/20070212-2.c`

These are backend correctness failures past object emission and link. They
should not be treated as classification-only progress.

## In Scope

- Triage the 34 runtime mismatches into stable lowering/runtime buckets from
  object dumps, qemu behavior, and minimized representative cases.
- Repair backend lowering bugs that cause c4c-generated RV64 code to abort or
  segfault when clang exits successfully.
- Split distinct semantic families into smaller follow-up ideas if this bucket
  proves too broad.

## Out Of Scope

- Making runtime failures disappear by weakening comparison contracts.
- Treating compile-fail or timeout cases as part of this runtime bucket.
- Large unrelated rewrites before the runtime mismatch families are
  classified.

## Acceptance

- The 34 runtime mismatches are classified into concrete root-cause families.
- At least the representative abort and segfault cases either pass or have
  precise child ideas with proof commands.
- A refreshed RV64 gcc_torture backend subset shows no increase in runtime
  mismatches.

## Reviewer Reject Signals

- Reject changes that only make a representative return 0 without preserving
  the program's observable semantics.
- Reject expectation rewrites, qemu comparison weakening, or allowlist
  filtering.
- Reject claiming progress from compile-only proof when the case reaches qemu.
- Reject keeping abort and segfault failures lumped together after evidence
  shows separate root causes that need separate owners.
