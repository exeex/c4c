# AArch64 Call-Boundary Aggregate-Lane Record Schema Cleanup

## Goal

Make the aggregate-lane shape carried by `CallBoundaryMoveInstructionRecord`
explicit through a narrow schema or validated-accessor cleanup while preserving
all existing call-boundary move behavior, aggregate-lane publication behavior,
printer output, and diagnostics.

## Why This Exists

The call-boundary record/printer audit found that one generic
`CallBoundaryMoveInstructionRecord` currently carries ordinary call-boundary
moves, value and preservation moves, immediate publication, f128 facts,
prepared frame-slot memory, and aggregate register-lane publication. The
aggregate-lane path depends on generic source-memory, destination-register, and
provenance fields plus `is_aggregate_register_lane_publication` to revalidate
phase, move reason, prepared non-address source memory, size range, and X-view
ABI GPR destination. That is a concrete schema cleanup boundary if the route
keeps semantics unchanged.

## In Scope

- Audit `CallBoundaryMoveInstructionRecord` and
  `is_aggregate_register_lane_publication` for a narrow explicit
  aggregate-lane record shape, tagged sub-schema, or validated accessor helper.
- Preserve ordinary call-boundary moves, value publication, preservation moves,
  immediate publication, f128 call-boundary facts, prepared frame-slot memory,
  and aggregate register-lane publication.
- Keep `calls.cpp` responsible for AArch64 ABI construction, selected-node
  facts, prepared-source decisions, scratch/preservation material, and whether
  a move remains a call-boundary record.
- Keep `machine_printer.cpp` responsible for final printable assembly
  constraints, diagnostics, register/immediate/memory presence checks, scratch
  selection, materialized-address emission, and aggregate-lane print routing.
- Prove that aggregate-lane dispatch and ordinary call-boundary printing still
  accept and reject the same record shapes as before.

## Out Of Scope

- Splitting all call-boundary record families or redesigning every
  call-boundary move variant.
- Moving AArch64 ABI construction, source/destination selection, prepared
  frame-slot memory authority, or scratch decisions out of `calls.cpp`.
- Deleting printer-side validation because schema/accessor validation exists.
- Changing assembly output, diagnostics, unsupported-path behavior, selection
  status, or record-surface classification.
- Reworking `CallBoundaryAbiBindingInstructionRecord`, direct/indirect call
  records, or generic instruction/printer public entry points without separate
  evidence.

## Acceptance And Proof Expectations

- The implementation makes the aggregate-lane record shape clearer without
  changing the accepted or rejected shapes for ordinary call-boundary moves or
  aggregate register-lane publication.
- Focused proof covers ordinary call-argument and return moves, value and
  preservation publication, immediate call-argument publication, f128 carrier
  cases, prepared frame-slot memory moves, aggregate register-lane publication,
  and aggregate-lane rejection paths.
- Build proof includes the AArch64 backend targets affected by
  `instruction.*`, `calls.cpp`, and `machine_printer.cpp`.
- Acceptance-sized slices should use matching before/after logs when the
  record schema, helper predicates, or public instruction declarations change.

## Reviewer Reject Signals

- The route claims schema progress through field renames, accessor wrappers, or
  expectation rewrites while the same ambiguous aggregate-lane shape remains
  encoded in the same generic fields.
- Printer validation for ordinary call-boundary moves or aggregate-lane
  publication is deleted or weakened because a schema helper now exists.
- AArch64 ABI construction, source/destination selection, prepared-source
  authority, or scratch decisions move from `calls.cpp` into generic schema
  helpers.
- The implementation changes assembly output, unsupported diagnostics,
  selection status, or record-surface classification while claiming a no
  semantics cleanup.
- Proof covers only one aggregate-lane publication case while ordinary
  call-boundary moves, value/preservation moves, immediate publication, f128
  cases, prepared frame-slot memory, and rejection paths remain unexamined.
- Work on ABI binding records, direct/indirect call records, or broad printer
  entry points is swept into this idea without a separate traced record path.
