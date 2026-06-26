# RV64 Object Route Aggregate `va_arg` Helper Lowering

Status: Closed
Type: Target ABI follow-up
Parent: `ideas/open/354_rv64_gcc_torture_prepared_module_shape_classification.md`

## Goal

Lower the RV64 aggregate `va_arg` helper path from explicit prepared vararg
helper facts.

## Why This Exists

Ideas 360 through 367 closed the shared vararg contract, scalar helper, and
`va_start` entry-state prerequisites. Idea 366's closure note recorded that
`src/920908-1.c` advanced to this later boundary, and idea 354's Step 3
representative refresh confirms the current stop:

```text
unsupported_variadic_helper_lowering: RV64 object route does not yet lower va_arg_aggregate helper
```

Representative:

- `src/920908-1.c`

This is remaining RV64 target ABI helper lowering, not shared LIR/BIR vararg
contract discovery.

## In Scope

- Inspect prepared `va_arg_aggregate` helper facts, including source `va_list`,
  destination payload, and aggregate access plan.
- Define the supported RV64 aggregate `va_arg` movement plan for the first safe
  class of aggregate payloads.
- Implement object-route lowering only from explicit prepared helper facts and
  target ABI metadata.
- Preserve precise unsupported diagnostics for aggregate classes or ABI paths
  that remain unsupported.
- Add focused tests for supported aggregate `va_arg` lowering and fail-closed
  unsupported shapes.
- Rerun `src/920908-1.c` and document its outcome.

## Out of Scope

- Reopening shared LIR/BIR vararg semantics from idea 360.
- Reopening `va_start` destination-address or overflow-area setup from ideas
  365 and 366.
- Byval aggregate parameter-home lowering for non-`va_arg` entry parameters.
- Inferring aggregate layout or helper resources from source syntax rather than
  prepared helper facts.

## Acceptance Criteria

- Focused backend tests prove the supported `va_arg_aggregate` helper lowering
  or a more precise unsupported boundary.
- `src/920908-1.c` is rerun and its next boundary or pass result is documented.
- Existing RV64 variadic helper, prepared-printer, and object-emission coverage
  remains green.
- Progress consumes the explicit prepared helper contract instead of adding
  testcase-specific lowering.

## Reviewer Reject Signals

- Reject named-case-only handling for `src/920908-1.c`.
- Reject object-emission guesses about aggregate layout, `va_list` state, or
  helper resources not present in prepared helper facts.
- Reject expectation downgrades, skip broadening, or allowlist filtering
  claimed as aggregate `va_arg` progress.
- Reject broad variadic rewrites that do not directly prove aggregate helper
  lowering and fail-closed unsupported behavior.
- Reject diagnostic-only renames that keep the same unsupported
  `va_arg_aggregate` helper boundary.

## Closure Notes

Closed after the committed lowering slices:

- `2b670bd1 add aggregate va_arg helper contract coverage`
- `cfb28244 publish aggregate va_arg payload write address`
- `cea00708 lower RV64 aggregate va_arg overflow helper`

The RV64 object route now lowers the first supported overflow-area aggregate
`va_arg` helper shape from explicit prepared facts: source `va_list` home,
overflow pointer field, `payload_write_address`, copy size/alignment, stride,
and frame resources. Focused backend coverage proves both the supported copy
path and fail-closed diagnostics when required helper facts are absent.

`src/920908-1.c` and
`tests/backend/case/riscv64_variadic_aggregate_overflow_helper_contract.c`
advance past the prior aggregate helper boundary:

```text
unsupported_variadic_helper_lowering: RV64 object route does not yet lower va_arg_aggregate helper
```

They now expose the separate entry-parameter home boundary:

```text
unsupported_param_home: RV64 object route requires all parameters in supported GPR or prepared FPR register homes
```

That boundary is not aggregate `va_arg` helper lowering. It remains outside
this idea's scope and is already owned by the open follow-up
`ideas/open/374_rv64_object_route_non_register_param_homes.md`; no duplicate
open idea was created.

Close-time regression guard used matching backend logs:

- `test_before.log`: passed=326 failed=0 total=326
- `test_after.log`: passed=326 failed=0 total=326
- no newly failing tests
- accepted with `--allow-non-decreasing-passed` because this lifecycle-only
  close follows the already accepted backend roll-forward and does not change
  implementation code
