# 110 AArch64 Remaining Baseline Failure Recovery

## Context

The latest preserved full-baseline log still reports these AArch64 failures:

- `backend_aarch64_instruction_dispatch`
- `c_testsuite_aarch64_backend_src_00172_c`
- `c_testsuite_aarch64_backend_src_00180_c`
- `c_testsuite_aarch64_backend_src_00216_c`
- `c_testsuite_aarch64_backend_src_00220_c`
- `c_testsuite_aarch64_backend_src_00204_c`

That list is visible in `test_baseline.log` and
`log/baseline_93a0ca1a852132daa7ed40d61055b20c95aedd88.log`.  An older
baseline log, `log/baseline_9f348520108810254973de09888a576c1a81d41d.log`,
already showed the same failure family with shifted CTest numbers, so this is
not a new drift introduced by the later 00204-focused repair slice.

A current targeted rerun of the named tests after the 00204 repair showed
`c_testsuite_aarch64_backend_src_00204_c` passing.  The retained baseline log is
therefore stale for 00204, while the other five failures remain current.

## Goal

Recover the current remaining AArch64 baseline failures before resuming broader
BIR/prealloc cleanup work.

## In Scope

- Re-run and characterize the current failure set:
  - `backend_aarch64_instruction_dispatch`
  - `c_testsuite_aarch64_backend_src_00172_c`
  - `c_testsuite_aarch64_backend_src_00180_c`
  - `c_testsuite_aarch64_backend_src_00216_c`
  - `c_testsuite_aarch64_backend_src_00220_c`
- Confirm whether 00204 remains green on the same proving subset and refresh
  the baseline record only after the actual current result is understood.
- Split the failures by real lowering capability, not by testcase name.  Early
  clues from the targeted run suggest:
  - instruction-dispatch coverage around selected `f64` global readback feeding
    a call ABI move
  - C tests covering pointer/string mutation, aggregate initialization or copy
    shape, local aggregate ordering, and one current segmentation fault
- Fix semantic lowering or data-flow ownership defects without weakening test
  expectations.

## Out of Scope

- BIR/prealloc legacy residue audit work from idea 109.
- x86 or RISC-V backend work.
- Expectation downgrades, testcase-shaped special cases, or unsupported-route
  rewrites.

## Proof

Use a narrow current proving command first:

```sh
ctest --test-dir build -j1 --output-on-failure -R '^(backend_aarch64_instruction_dispatch|c_testsuite_aarch64_backend_src_00(172|180|216|220|204)_c)$'
```

When the narrow set is clean, refresh the broader AArch64 baseline with the
repo-standard baseline command and compare against the preserved baseline log.

## Closure Notes

- Closure should state which failures were genuinely fixed and whether 00204 was
  only stale in the preserved baseline log.
- If a failure cluster turns out to require a larger architectural move, close
  or split this idea with follow-up ideas rather than expanding this recovery
  into unrelated BIR/prealloc cleanup.
