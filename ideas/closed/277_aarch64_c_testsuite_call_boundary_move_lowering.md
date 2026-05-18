# AArch64 C-Testsuite Call-Boundary Move Lowering

Status: Closed
Created: 2026-05-18
Closed: 2026-05-18

## Intent

Repair the AArch64 backend capability family where c-testsuite backend-route
cases reach call-boundary move lowering or selection and then surface at the
machine-node printer as `deferred_unsupported`.

## Why This Exists

The parked AArch64 c-testsuite runtime-route plan produced a broader failure
inventory after route registration. Runtime execution is still blocked on this
host by missing `C_TESTSUITE_AARCH64_BACKEND_RUNNER`, but the same inventory
also exposed non-runtime frontend/backend failures. The largest named
non-runtime follow-up from that review is call-boundary move
lowering/selection, which should be handled as backend capability work rather
than as more route-registration or classification churn.

## In Scope

- Identify representative failing c-testsuite cases from the AArch64 backend
  scan where call-boundary move lowering or selection reaches the machine-node
  printer as `deferred_unsupported`.
- Trace the owned lowering path from the relevant BIR/LIR facts through AArch64
  move materialization, call argument or result boundary handling, and machine
  node printing.
- Add semantic lowering/selection support for the missing call-boundary move
  forms.
- Preserve stage-specific diagnostics so route failures remain attributable to
  frontend, backend, assembler/link, runtime unavailable, runtime nonzero, or
  runtime mismatch.
- Prove the repair with focused backend tests and the relevant AArch64
  c-testsuite backend subset.

## Out of Scope

- Configuring an AArch64 runtime runner or claiming runtime pass evidence.
- Weakening c-testsuite expected outputs, allowlists, or CTest expectations.
- Reclassifying backend failures as unsupported without repairing the lowering
  capability.
- Adding filename-specific or named-case printer shortcuts.
- Broad rewrites of unrelated AArch64 instruction selection, ABI, or frontend
  lowering paths.

## Acceptance Criteria

- Representative call-boundary move failures no longer reach the printer as
  `deferred_unsupported`.
- The fix is expressed as a general lowering or selection rule for the missing
  move forms, not as c-testsuite filename matching.
- Focused backend proof demonstrates the repaired lowering path.
- The AArch64 c-testsuite backend subset shows the targeted failure family is
  reduced or reclassified to the next truthful owner layer without counting
  `[RUNTIME_UNAVAILABLE]` as pass evidence.

## Closure Notes

Closed after Step 6 of the active runbook. Focused backend proof passed for the
symbol-address call-boundary move form, and the broader AArch64 backend
c-testsuite scan reported zero occurrences of the old diagnostic:

`deferred_unsupported: call-boundary move node requires prepared register source and destination`

The broad scan still failed overall because this host lacks an AArch64 runtime
runner and because separate frontend/backend capability families remain. Those
remaining blockers were not counted as runtime pass evidence and are outside
this idea's acceptance criteria.

Durable follow-up: undefined `.LBB...` temporary label emission is tracked
separately in `ideas/open/278_aarch64_c_testsuite_undefined_temporary_labels.md`.

## Reviewer Reject Signals

Reject the route or slice if it:

- matches c-testsuite filenames, exact case names, or one-off call shapes
  instead of repairing call-boundary move semantics;
- only rewrites expectations, allowlists, or unsupported classifications while
  the same `deferred_unsupported` printer failure remains;
- claims runtime capability progress without an AArch64 runner or native host;
- collapses backend, assembler/link, and runtime failures into one generic
  failure bucket;
- broadens AArch64 lowering or ABI behavior without focused proof for the
  changed call-boundary move forms;
- hides missing frontend/BIR facts inside target-specific printer code.
