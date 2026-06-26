# RV64 Object Route Stack Frame And Parameter Home Edges

Status: Open
Type: Follow-up repair idea
Parent: `ideas/open/354_rv64_gcc_torture_prepared_module_shape_classification.md`

## Goal

Repair RV64 prepared stack-frame, callee-saved GPR, parameter-home, and closely
related function-admission edges that still block object emission.

## Why This Exists

The 2026-06-26 reopened 354 classification found 103 failures in this ABI
admission area:

- 66 `RV64 object route requires supported prepared callee-saved GPR save slots`
- 14 `RV64 object route requires a supported prepared stack frame`
- 22 `RV64 object route requires all parameters in supported GPR or prepared FPR register homes`
- 1 variadic function admission failure for missing `va_start` destination
  `va_list` address facts

Representatives:

- `tests/c/external/gcc_torture/src/930603-1.c`
- `tests/c/external/gcc_torture/src/20001017-1.c`
- `tests/c/external/gcc_torture/src/va-arg-21.c`

## In Scope

- Classify frame and home facts missing from the prepared object contract.
- Add RV64 lowering/admission for supported callee-saved GPR save slots,
  prepared stack frames, GPR/FPR parameter homes, and the narrow current
  variadic admission edge when facts already exist.
- Split any producer-side missing fact into a separate idea instead of hiding
  it in target emission.

## Out Of Scope

- Reimplementing the full ABI classifier in the object emitter.
- Reopening already-closed byval/sret/va_list ideas unless the latest logs
  prove a new fact boundary.
- Treating semantic function-signature failures as this backend bucket.

## Acceptance

- Representative stack/frame/home cases no longer fail with the same
  unsupported stack-frame or param-home diagnostics.
- Any remaining producer-side missing fact is documented and split instead of
  absorbed into RV64 emission.
- A refreshed subset shows this bucket count decreases without adding runtime
  aborts or segfaults.

## Reviewer Reject Signals

- Reject hard-coded fixes for `930603-1.c`, `20001017-1.c`, or `va-arg-21.c`.
- Reject lowering that fabricates home facts not present in the prepared
  contract without documenting and repairing the producer boundary.
- Reject ABI changes that pass object compilation but misplace parameters,
  callee-saved values, or variadic `va_list` state at runtime.
- Reject expectation downgrades or allowlist filtering.
