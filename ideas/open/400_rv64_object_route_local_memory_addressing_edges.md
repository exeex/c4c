# RV64 Object Route Local Memory Addressing Edges

Status: Open
Type: Follow-up repair idea
Parent: `ideas/open/354_rv64_gcc_torture_prepared_module_shape_classification.md`

## Goal

Repair current RV64 object-route failures for prepared local memory base,
offset, and access-width forms.

## Why This Exists

The 2026-06-26 reopened 354 classification found 30 local-memory object-route
failures:

- 26 `RV64 object route requires prepared frame-slot or pointer-value base-plus-offset local memory addressing`
- 4 `RV64 object route supports only 1-, 2-, 4-, and 8-byte prepared local memory accesses`

Representatives:

- `tests/c/external/gcc_torture/src/20000722-1.c`
- `tests/c/external/gcc_torture/src/20030910-1.c`

## In Scope

- Classify local-memory base kinds and widths in the current bucket.
- Add RV64 object lowering for reusable prepared base-plus-offset and width
  forms that are valid under the prepared contract.
- Split producer-side missing local-memory facts when the prepared handoff is
  insufficient.
- If the required fix changes local-memory fact production, aliasing, or base
  classification in BIR/prepared lowering, create or switch to a separate
  BIR/prepared idea instead of patching MIR/RV64 emission around missing facts.

## Out Of Scope

- Semantic `lir_to_bir` local-memory failures that occur before prepared
  object handoff.
- Treating compound-literal or aggregate cases as named special cases.
- Weakening memory access width diagnostics without adding lowering.

## Acceptance

- Representative local-memory cases no longer fail with the same object-route
  local-memory diagnostics.
- A refreshed subset shows fewer local-memory object-route failures.
- Runtime comparison is green for newly object-emitted local-memory cases.

## Reviewer Reject Signals

- Reject filename-specific handling for `20000722-1.c` or `20030910-1.c`.
- Reject lowering that assumes an arbitrary stack slot layout not described by
  prepared facts.
- Reject MIR/RV64 changes that synthesize local-memory base, offset, width, or
  alias facts that should have been published by BIR/prepared lowering.
- Reject changes that turn a local-memory object-route failure into a silent
  runtime mismatch without diagnosing the new root.
- Reject expectation rewrites or allowlist filtering.
