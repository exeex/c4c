# RV64 Object Route Prepared Move-Bundle Target Shapes

Status: Open
Type: Repair idea
Parent: `ideas/open/354_rv64_gcc_torture_prepared_module_shape_classification.md`
Split From: `ideas/closed/375_rv64_object_route_scalar_compare_trunc_lowering.md`

## Goal

Admit prepared move-bundle target shapes needed by the RV64 object route after
scalar compare/trunc lowering has produced supported value homes.

## Why This Exists

Idea 375 repaired the scalar compare-result truncation blocker for
`src/20000217-1.c`. Its representative rerun advanced to a distinct prepared
move-bundle target-shape rejection:

```text
unsupported_move_bundle_target_shape: prepared move bundle requires unsupported RV64 moves
```

Evidence:

- `build/agent_state/375_step3_20000217-1.runner.log`
- `build/rv64_gcc_c_torture_backend/src_20000217-1.c/case.log`

The earlier compare/trunc audit identified the representative publication flow
as a compare result in a GPR, an `i32 -> i16` trunc publication to a stack
slot, and a return publication through `a0`. This follow-up owns only the
remaining prepared move-bundle target-shape gap, not the compare/trunc
materialization that idea 375 closed.

## In Scope

- Audit the prepared move bundle rejected by `src/20000217-1.c` after idea 375.
- Identify the first supportable RV64 object-route move target shape using
  explicit prepared move source and destination facts.
- Implement semantic RV64 object emission for that prepared move shape when the
  source and destination homes are supported scalar integer locations.
- Preserve existing fail-closed diagnostics for unsupported move sources,
  destinations, widths, phases, or ambiguous move bundles.
- Add focused backend tests for the admitted move-bundle target shape and
  nearby rejected shapes.
- Rerun `src/20000217-1.c` and record whether it passes or advances to the next
  out-of-scope owner.

## Out of Scope

- Reopening scalar compare/trunc lowering; idea 375 closed that route.
- Frame-slot address call-argument materialization; idea 372 closed that route.
- Pointer-value local-memory loads/stores; idea 368 closed that route.
- Frame-slot payload-value call arguments; idea 373 closed that route.
- Non-register parameter homes; use
  `ideas/open/374_rv64_object_route_non_register_param_homes.md`.
- Aggregate `va_arg`, byval aggregate homes, terminator lowering, CFG
  reconstruction, globals, strings, or data-section ownership.
- Inferring move semantics from testcase names or source syntax.

## Acceptance Criteria

- Focused RV64 object-emission tests prove the selected prepared move-bundle
  target shape.
- Unsupported adjacent move-bundle shapes keep precise diagnostics.
- `src/20000217-1.c` is rerun and either passes or advances to a documented
  next owner because of semantic move-bundle target repair.
- Existing focused backend object-emission and prepared-contract coverage
  remains green.

## Reviewer Reject Signals

- Reject testcase-name shortcuts for `src/20000217-1.c` or its exact return
  expression.
- Reject hard-coded value ids, frame slots, registers, bundle phases, or
  instruction indexes not derived from prepared move metadata.
- Reject changes that only rename diagnostics or rewrite expectations while
  leaving the same prepared move-bundle target shape unsupported.
- Reject broad terminator, call-argument, parameter-home, local-memory,
  globals, strings, or CFG rewrites inside this idea.
- Reject expectation downgrades, allowlist filtering, unsupported-test
  weakening, or classification-only changes claimed as capability progress.
