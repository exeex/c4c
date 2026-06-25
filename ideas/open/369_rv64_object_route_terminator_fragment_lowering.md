# RV64 Object Route Terminator Fragment Lowering

Status: Open
Type: Repair idea
Parent: `ideas/open/354_rv64_gcc_torture_prepared_module_shape_classification.md`

## Goal

Extend RV64 object-route lowering for prepared BIR terminator fragments that
remain unsupported after multi-block text and data/string support.

## Why This Exists

Idea 357 closed the data-section/global/string milestone after representatives
advanced past data, symbol, string, and relocation failures. Idea 354's Step 3
representative refresh shows two of those data/string representatives now stop
at:

```text
unsupported_terminator_fragment: BIR terminator requires unsupported RV64 object lowering
```

Representatives:

- `src/20000224-1.c`
- `src/20000112-1.c`

This is a later control-flow/terminator lowering bucket, not a data-section or
string constant bucket.

## In Scope

- Inspect the prepared BIR terminators used by the representative failures.
- Identify which terminator fragment shape is supportable under the current
  RV64 object route.
- Implement semantic RV64 object lowering for that terminator shape using
  prepared CFG and publication facts.
- Preserve precise unsupported diagnostics for terminator classes that remain
  outside the supported subset.
- Add focused tests that cover the supported terminator shape and fail-closed
  unsupported shapes.
- Rerun the listed representative cases and record their outcomes.

## Out of Scope

- Reopening data-section, global-symbol, string-constant, or relocation support
  from idea 357.
- Reconstructing CFG, branch targets, or value publication by scanning source
  syntax.
- Broad assembler/object-route replacement unrelated to the observed terminator
  fragment.
- Frame-slot base-plus-offset local memory, byval parameter homes, or variadic
  helper lowering.

## Acceptance Criteria

- Focused backend tests prove the newly supported terminator fragment in RV64
  object emission.
- Unsupported terminator shapes still produce precise diagnostics.
- `src/20000224-1.c` and `src/20000112-1.c` are rerun and their next boundary
  or pass result is documented.
- Existing focused object-emission and prepared control-flow coverage remains
  green.

## Reviewer Reject Signals

- Reject testcase-name shortcuts for `src/20000224-1.c`, `src/20000112-1.c`, or
  their exact source patterns.
- Reject changes that infer CFG or branch destinations outside the prepared
  module contract.
- Reject broad terminator rewrites that do not prove the specific supported
  fragment and fail-closed behavior.
- Reject expectation downgrades, skip broadening, or scan filtering claimed as
  progress.
- Reject diagnostic-only renames that leave the same unsupported terminator
  fragment unexplained.
