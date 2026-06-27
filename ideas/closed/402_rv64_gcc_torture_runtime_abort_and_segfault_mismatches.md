# RV64 gcc_torture Runtime Abort And Segfault Mismatches

Status: Closed
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
- If triage shows the runtime mismatch is caused by incorrect BIR/prepared
  facts rather than RV64 lowering of correct facts, create or switch to a
  separate BIR/prepared idea before changing MIR/RV64 code.

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
- Reject MIR/RV64 fixes that mask bad BIR/prepared facts instead of routing
  the producer defect to a separate semantic idea.
- Reject keeping abort and segfault failures lumped together after evidence
  shows separate root causes that need separate owners.

## Lifecycle Notes

- 2026-06-27: Closed after runtime representative triage split the active
  segfault and abort families. The `20070212-2.c` frame-slot local-address
  segfault family was repaired by implementation commit `9b2ff12e`, and Step 3
  proof showed `dump_bir_status=0`, `prepared_status=0`,
  `c4c_bin_objdump_status=0`, `clang_qemu_status=0`, `c4c_qemu_status=0`, and
  `c4c_qemu_strace_status=0`.
- 2026-06-27: The repaired `20070212-2.c` proof still publishes explicit
  frame-slot address materialization facts for `%lv.param.i1` and
  `%lv.param.j1`, and object evidence materializes those addresses before
  storing `%lv.f1`.
- 2026-06-27: The remaining `20000113-1.c` abort is a distinct runtime owner:
  `dump_bir_status=0`, `prepared_status=0`, `c4c_bin_objdump_status=0`,
  `clang_qemu_status=0`, `c4c_qemu_status=134`, and
  `c4c_qemu_strace_status=134`. It is routed to
  `ideas/open/411_rv64_object_route_rhs_bitfield_boolean_select_abort.md`.
- 2026-06-27: Close gate passed with the backend regression guard over
  `ctest --test-dir build -j --output-on-failure -R '^backend_'`. The
  rolled-forward `test_before.log` and regenerated `test_after.log` both
  reported 326/326 passing backend tests with no new failures.
