# Current Packet

Status: Active
Source Idea Path: ideas/open/87_aarch64_call_boundary_record_printer_surface_audit.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Prepare Audit Close Summary

## Just Finished

Completed Step 5, "Prepare Audit Close Summary", by recording the
close-ready audit outcome for source idea
`ideas/open/87_aarch64_call_boundary_record_printer_surface_audit.md`.

Call-boundary record surfaces and ownership:

- `calls.cpp` owns AArch64 ABI construction for call arguments, returns,
  value and preservation moves, immediate publication, f128 facts, prepared
  frame-slot memory, scratch/preservation material, and selected-node facts.
- `instruction.*` owns the machine-record schema and helper predicates that
  carry call-boundary move facts to the printer, especially
  `CallBoundaryMoveInstructionRecord` and the aggregate-lane predicate layered
  over its generic source, destination, and provenance fields.
- `machine_printer.cpp` owns final printable assembly constraints,
  diagnostics, register/immediate/memory presence checks, scratch selection,
  materialized-address emission, and dispatch between ordinary call-boundary
  printing and aggregate-lane publication printing.

Aggregate-lane record surfaces and ownership:

- Register-lane publication shares the call-boundary move record carrier, but
  has distinct aggregate ownership around byval lane publication, prepared
  source memory, destination-lane derivation, chunk-width load selection,
  scratch exclusion, and printer rejection paths.
- The local aggregate helper/table surface in `instruction.cpp` covers load
  mnemonics, printable chunk selection, chunk printability checks,
  destination-lane derivation, aggregate lane memory recognition, and scratch
  exclusion; this is a local contraction candidate, not a transfer of byval
  ABI authority or printer diagnostics.
- Stack-lane inline-asm publication remains aggregate-specific construction
  ownership and was not classified as the same record/printer boundary as
  register-lane publication.

Concrete follow-up ideas created:

- `ideas/open/90_aarch64_aggregate_lane_helper_table_contraction.md` captures
  the local aggregate-lane helper/table contraction candidate with proof
  expectations for register-sourced and frame-slot-sourced byval lanes,
  chunk-width load selection, destination-lane derivation, scratch exclusion,
  and printer rejection paths.
- `ideas/open/91_aarch64_call_boundary_aggregate_lane_record_schema_cleanup.md`
  captures the schema cleanup candidate for making the aggregate-lane shape in
  `CallBoundaryMoveInstructionRecord` explicit while preserving ordinary
  call-boundary moves, value/preservation moves, immediate publication, f128
  cases, prepared frame-slot memory, aggregate register-lane publication, and
  rejection paths.

Rejected candidates and blocking evidence:

- Target-local ABI/printer ownership surfaces were not converted into
  implementation ideas because the audit evidence ties them to AArch64 ABI
  phase decisions, diagnostics, scratch choices, selected-node facts, record
  construction, final printable assembly constraints, or printer-owned
  rejection behavior.
- `CallBoundaryAbiBindingInstructionRecord`, direct/indirect call records,
  broad surface identifiers, and public printer entry points lacked a traced
  no-semantics cleanup boundary, so they remain `missing-evidence`.
- Frame-slot load/address materialization checks remain split between
  selection status and printing; the audit did not prove a stable helper or
  schema boundary that could be changed without moving ownership.

No backend tests were required for this packet because Step 5 changed only
`todo.md` audit/lifecycle state and did not change implementation files.

## Suggested Next

Supervisor can route lifecycle closure or deactivate/switch the active plan;
implementation work should proceed through follow-up idea 90 or 91, not by
expanding source idea 87.

## Watchouts

- Do not close idea 87 by implying implementation capability changed; the
  completed work is an audit route with two separate implementation ideas.
- Do not claim cleanup by deleting printer validation or moving AArch64 ABI
  call-boundary construction into shared authority.
- Do not add new implementation scope for `missing-evidence` surfaces without a
  separate traced source idea.

## Proof

Audit-only proof command for Step 5:

```sh
printf 'Audit-only Step 5; no backend tests required.\n' > test_after.log && git diff --name-only >> test_after.log && if git diff --name-only | rg -q '^src/backend/mir/aarch64/codegen/(calls|instruction|machine_printer)\.(cpp|hpp)$|^plan\.md$|^ideas/'; then printf 'ERROR: non-todo file changed during audit close-summary packet.\n' >> test_after.log; exit 1; fi
```

Result: passed; `test_after.log` contains the audit-only Step 5 marker and
dirty file list. No implementation files, `plan.md`, or `ideas/open/*` files
changed during this packet.
