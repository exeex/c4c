# RV64 Object Route Local Memory Addressing Edges

Status: Closed
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
- `tests/c/external/gcc_torture/src/20020225-2.c` after scalar/floating edge
  lowering moved it past `unsupported_floating_cast`

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

## Lifecycle Notes

- 2026-06-26: Activated after closing
  `ideas/closed/401_rv64_object_route_scalar_and_floating_edge_lowering.md`.
  The inherited `src/20020225-2.c` representative no longer fails with
  `unsupported_floating_cast`; it now reaches
  `unsupported_local_memory_access` with an 8-byte `double` store to prepared
  local union storage whose frame-slot range is reported out of bounds. Treat
  this as local-memory/addressing work or split a producer fact repair; do not
  compensate in floating-cast lowering.
- 2026-06-26: Closed after commit `656502db` lowered coherent prepared F64
  local-memory frame-slot and pointer-value accesses. The three-case probe now
  shows `src/20030910-1.c` moved off local-memory diagnostics to the existing
  terminator-fragment bucket owned by
  `ideas/open/396_rv64_object_route_terminator_fragment_lowering_refresh.md`.
  The remaining local-memory diagnostics are not target-emission gaps:
  `src/20020225-2.c` is split to
  `ideas/open/405_prepared_local_aggregate_slot_layout_facts.md` for
  contradictory one-byte union slot/layout publication, and `src/20000722-1.c`
  is routed to `ideas/open/399_rv64_object_route_global_data_and_symbol_memory.md`
  for string/global symbol address materialization with missing structured
  symbol identity.
- Close gate: backend regression guard passed on 2026-06-26 with
  `test_before.log` and `test_after.log` both at 326/326 passing tests using
  `ctest --test-dir build -j --output-on-failure -R '^backend_'`.
