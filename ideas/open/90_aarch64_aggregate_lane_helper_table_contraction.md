# AArch64 Aggregate-Lane Helper/Table Contraction

## Goal

Contract the AArch64 aggregate register-lane helper and table surfaces around
byval lane publication while preserving the current construction behavior,
machine-printer assembly output, diagnostics, scratch choices, and chunk-width
selection.

## Why This Exists

The call-boundary record/printer audit found a concrete local contraction
candidate in `instruction.cpp`: aggregate-lane load mnemonics, printable chunk
selection, chunk printability checks, destination-lane derivation, aggregate
lane memory helpers, and scratch exclusion are table/helper-shaped surfaces
already consumed by both construction and printing. This is local AArch64
aggregate-lane spelling/validation cleanup, not a reason to move byval ABI
authority or printer diagnostics elsewhere.

## In Scope

- Audit and contract the local aggregate-lane helper/table surface in
  `src/backend/mir/aarch64/codegen/instruction.cpp` and its existing public
  declarations only where needed by current construction and printer paths.
- Preserve helper responsibilities for load mnemonic selection, printable
  chunk selection, chunk printability, destination-lane derivation, aggregate
  lane memory recognition, and scratch exclusion.
- Keep register-sourced and prepared-frame-slot aggregate lane publication
  behavior equivalent for AArch64 byval lanes.
- Keep `print_aggregate_register_lane_publication_lines` responsible for final
  emitted assembly spelling, scratch use, OR assembly, and byte-shifted lane
  placement.
- Prove unchanged output and diagnostics for aggregate register-lane
  publication paths that use the helper/table surface.

## Out Of Scope

- Moving AArch64 byval lane classification, selected-lane extent, prepared
  source validation, lane/chunk coverage, or size-limit authority out of
  `calls.cpp`.
- Folding stack-lane inline-asm publication into register-lane printer
  cleanup.
- Replacing printer-owned rejection checks for missing scratch, missing
  destination lane, missing printable source chunk, or missing chunk load
  mnemonic.
- Broad call-boundary, aggregate-lane, or machine-printer rewrites beyond this
  local helper/table contraction.
- Changing assembly output, unsupported-path contracts, diagnostics, or scratch
  selection.

## Acceptance And Proof Expectations

- The implementation leaves aggregate-lane construction and printer behavior
  semantically unchanged while reducing duplicated local helper/table spelling.
- Focused proof covers register-sourced byval lane publication,
  frame-slot-sourced byval lane publication, chunk-width load selection,
  destination-lane derivation, scratch exclusion, and printer rejection paths.
- Build proof includes the AArch64 backend targets affected by
  `instruction.*`, `calls.cpp`, and `machine_printer.cpp`.
- Acceptance-sized slices should use matching before/after logs when the
  helper/table surface or public declarations move.

## Reviewer Reject Signals

- The route claims progress through helper renames, file movement, or table
  reshaping while aggregate-lane assembly output and diagnostics are not proven
  equivalent.
- AArch64 byval classification, selected-lane extent, prepared-source
  validation, chunk coverage, or size limits are moved away from the
  construction path without a separate source idea.
- Stack-lane inline-asm publication is folded into this idea as if it shared
  the same record/printer boundary as register-lane publication.
- Printer rejection checks for missing scratch, missing destination lane,
  missing printable source chunk, or missing chunk load mnemonic are deleted or
  weakened.
- Proof covers only one named byval lane case while nearby register-sourced,
  frame-slot-sourced, chunk-width, scratch, and rejection paths remain
  unexamined.
- The old duplicate helper/table spelling remains under a new abstraction name
  with no real contraction of the audited surface.
