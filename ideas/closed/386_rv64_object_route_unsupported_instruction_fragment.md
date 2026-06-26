# RV64 Object Route Frame-Slot Address Call Argument

Status: Closed
Type: Target lowering follow-up
Parent: `ideas/open/354_rv64_gcc_torture_prepared_module_shape_classification.md`
Exposed By: `ideas/closed/374_rv64_object_route_non_register_param_homes.md`
Split From: Step 1 evidence in the active runbook for the original
`unsupported_instruction_fragment` route.

## Goal

Identify and lower, or precisely route, the RV64 object-route same-module
`bir::CallInst` fragment where a scalar GPR pointer argument is selected from
an address-exposed local frame slot through
`PreparedCallArgumentSourceSelectionKind::FrameSlotAddress`.

The observed diagnostic is:

```text
unsupported_instruction_fragment: BIR instruction requires unsupported RV64 object lowering
```

## Why This Exists

The scalar GPR formal stack-slot home work closed the parameter-home gate for
the first supportable non-register formal-home shape. After that progress,
`tests/c/external/gcc_torture/src/va-arg-13.c` advanced to a later generic
object-route call-lowering boundary.

That stop is no longer specific to entry parameter homes. It needs its own
owner so future work can inspect the BIR instruction form, classify the missing
RV64 object lowering capability, and avoid stretching the completed
parameter-home idea.

Step 1 evidence also showed that
`tests/c/external/gcc_torture/src/920908-1.c` is a separate same-module
ABI memory-return/sret call family rejected by the
`call_plan->memory_return.has_value()` admission gate. That family is split to
`ideas/open/387_rv64_object_route_same_module_sret_calls.md` and should not be
implemented through this owner unless a reviewer proves both routes share an
existing common call-address materialization abstraction.

## In Scope

- Capture the exact prepared/BIR instruction fragment behind the
  `unsupported_instruction_fragment` diagnostic for the `va-arg-13.c`
  representative.
- Determine whether the frame-slot-address GPR call-argument operation is a
  supportable RV64 object-lowering rule or needs a narrower fail-closed
  diagnostic.
- Add focused backend coverage for the smallest semantic instruction fragment
  that can be supported from prepared facts.
- Preserve precise fail-closed diagnostics for instruction fragments that
  remain unsupported or require separate owners.
- Rerun the `va-arg-13.c` representative and record the next boundary if the
  fragment advances.

## Out of Scope

- Reopening scalar GPR stack-slot formal-home support from idea 374.
- Same-module ABI memory-return/sret calls, including the `920908-1.c`
  representative now tracked by idea 387.
- Byval aggregate parameter homes or aggregate `va_arg` helper lowering.
- Reconstructing ABI placement, stack layout, or source-level intent from
  testcase names or raw syntax.
- Broad RV64 object-route rewrites unrelated to the selected
  frame-slot-address GPR call-argument fragment.

## Acceptance Criteria

- The exact unsupported instruction fragment is identified from prepared/BIR
  evidence, not inferred from testcase names.
- Focused backend tests prove either semantic lowering for the selected
  instruction fragment or a narrower unsupported diagnostic.
- `va-arg-13.c` is rerun and documented against the new boundary.
- Any later boundary is routed to an existing or new owner instead of being
  folded silently into this idea.

## Closure Note

Closed after implementing guarded RV64 object-route lowering for GPR call
arguments selected by
`PreparedCallArgumentSourceSelectionKind::FrameSlotAddress`. Focused backend
coverage verifies that the emitted pointer payload uses the selected frame-slot
address materialization offset, not the ordinary scalar value home, and that
nearby malformed prepared facts fail closed.

`tests/c/external/gcc_torture/src/va-arg-13.c` now advances past c4c RV64
object compilation. The next boundary is a link failure on unresolved
`llvm.va_end.p0` in `test`; that follow-up is tracked separately in
`ideas/open/388_rv64_object_route_variadic_va_end_boundary.md`. Same-module
sret calls remain outside this idea and owned by
`ideas/open/387_rv64_object_route_same_module_sret_calls.md`.

## Reviewer Reject Signals

- Reject named-case-only handling for `va-arg-13.c`.
- Reject raw-string or shape matching that bypasses the semantic BIR
  instruction kind, operand, type, storage, and prepared object-route facts.
- Reject expectation downgrades, allowlist filtering, or diagnostic-only
  renames claimed as instruction-lowering progress.
- Reject reopening parameter-home logic from idea 374 unless fresh evidence
  shows the diagnostic is still caused by parameter-home facts.
- Reject implementing same-module memory-return/sret call support under this
  owner; that family belongs to idea 387 unless an existing shared abstraction
  is proven first.
- Reject broad RV64 lowering rewrites that do not directly prove the selected
  frame-slot-address GPR call-argument fragment and adjacent fail-closed cases.
