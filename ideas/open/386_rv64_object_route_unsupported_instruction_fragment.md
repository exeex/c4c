# RV64 Object Route Unsupported Instruction Fragment

Status: Open
Type: Target lowering follow-up
Parent: `ideas/open/354_rv64_gcc_torture_prepared_module_shape_classification.md`
Exposed By: `ideas/closed/374_rv64_object_route_non_register_param_homes.md`

## Goal

Identify and lower, or precisely route, the RV64 object-route BIR instruction
fragment that now fails with:

```text
unsupported_instruction_fragment: BIR instruction requires unsupported RV64 object lowering
```

## Why This Exists

The scalar GPR formal stack-slot home work closed the parameter-home gate for
the first supportable non-register formal-home shape. After that progress,
`tests/c/external/gcc_torture/src/va-arg-13.c` and
`tests/c/external/gcc_torture/src/920908-1.c` both advance to a later generic
object-route instruction-lowering boundary.

That stop is no longer specific to entry parameter homes. It needs its own
owner so future work can inspect the BIR instruction form, classify the missing
RV64 object lowering capability, and avoid stretching the completed
parameter-home idea.

## In Scope

- Capture the exact prepared/BIR instruction fragment behind the
  `unsupported_instruction_fragment` diagnostic for both known representatives.
- Determine whether the missing operation is a single RV64 object-lowering
  rule or multiple separable instruction families.
- Add focused backend coverage for the smallest semantic instruction fragment
  that can be supported from prepared facts.
- Preserve precise fail-closed diagnostics for instruction fragments that
  remain unsupported or require separate owners.
- Rerun the known representatives and record the next boundary if the fragment
  advances.

## Out of Scope

- Reopening scalar GPR stack-slot formal-home support from idea 374.
- Byval aggregate parameter homes, aggregate `va_arg` helper lowering, or
  frame-slot address call-argument materialization unless the inspected
  instruction fragment directly proves one of those existing owners is the
  correct route.
- Reconstructing ABI placement, stack layout, or source-level intent from
  testcase names or raw syntax.
- Broad RV64 object-route rewrites unrelated to the observed unsupported
  instruction fragment.

## Acceptance Criteria

- The exact unsupported instruction fragment is identified from prepared/BIR
  evidence, not inferred from testcase names.
- Focused backend tests prove either semantic lowering for the selected
  instruction fragment or a narrower unsupported diagnostic.
- `va-arg-13.c` and `920908-1.c` are rerun and documented against the new
  boundary.
- Any later boundary is routed to an existing or new owner instead of being
  folded silently into this idea.

## Reviewer Reject Signals

- Reject named-case-only handling for `va-arg-13.c` or `920908-1.c`.
- Reject raw-string or shape matching that bypasses the semantic BIR
  instruction kind, operand, type, storage, and prepared object-route facts.
- Reject expectation downgrades, allowlist filtering, or diagnostic-only
  renames claimed as instruction-lowering progress.
- Reject reopening parameter-home logic from idea 374 unless fresh evidence
  shows the diagnostic is still caused by parameter-home facts.
- Reject broad RV64 lowering rewrites that do not directly prove the observed
  unsupported instruction fragment and adjacent fail-closed cases.
