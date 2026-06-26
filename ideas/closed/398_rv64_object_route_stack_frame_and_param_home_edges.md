# RV64 Object Route Stack Frame And Parameter Home Edges

Status: Closed
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

## Lifecycle Notes

- 2026-06-26: Closed after refreshed Step 1 proof found no current owned RV64
  object-route stack-frame, callee-saved save-slot, parameter-home, or
  function-admission diagnostic in the selected seed bucket.
  `930603-1.c`, `20001017-1.c`, and `va-arg-21.c` all reported
  `c4cll_status=0` and `prepared_status=0`.
- 2026-06-26: Five stale-log nearby seeds (`20000412-2.c`, `20000706-2.c`,
  `20000717-4.c`, `20010605-2.c`, and `20011008-3.c`) stopped before
  prepared handoff with semantic LIR-to-BIR diagnostics, so they are not
  current 398 object-route evidence.
- 2026-06-26: Fresh prepared facts show explicit callee-saved save slots and
  frame facts for `930603-1.c`, explicit register-passed parameter homes for
  `20001017-1.c`, and currently admitted stack-passed formals. The remaining
  `va-arg-21.c` issue is producer-side missing
  `helper_operand_homes.va_start.destination_va_list_address` metadata, split
  into `ideas/open/408_prepared_va_start_destination_address_helper_operand_publication.md`.
- 2026-06-26: Close gate passed with the backend regression guard over
  `ctest --test-dir build -j --output-on-failure -R '^backend_'`. The
  rolled-forward `test_before.log` and regenerated `test_after.log` both
  reported 326/326 passing backend tests with no new failures.
