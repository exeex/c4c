Status: Active
Source Idea Path: ideas/open/02_aarch64_calls_emission_consolidation.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map The Printing And Effect-Publication Boundary

# Current Packet

## Just Finished

Step 1 mapped the surviving `calls_printing.cpp` boundary:

- Emission-node construction: `make_call_boundary_move_instruction`,
  `make_call_boundary_abi_binding_instruction`, and `make_call_instruction`.
  These build `InstructionRecord` payloads, select machine-node status, and
  publish operands/effects from prepared call facts.
- Pure machine-printer work: `print_call`, `print_call_boundary_move`, and
  `print_call_boundary_abi_binding`. These consume already-built target
  instruction records and spell assembly or unsupported diagnostics.
- Effect publication: `effect_from_prepared_call_preserved_value` and
  `effects_from_prepared_call_preserved_values`. These convert prepared
  preserved-value facts into `MachineEffectResource` records for
  `make_call_instruction`; they are not printer work.
- Duplicate prepared-call/effect decision logic: the selection helpers
  `call_boundary_move_selection_status`, `call_boundary_abi_binding_selection_status`,
  and `call_selection_status`, plus the preserved-value conversion path, still
  re-check prepared routes/provenance while constructing machine nodes.

## Suggested Next

Execute Step 2 by moving only the pure `print_call` helper into
`machine_printer.cpp` as machine-printer-owned call spelling, then remove its
exported declaration from `calls.hpp`. Keep `make_call_instruction` and
preserved-value effect publication in `calls_printing.cpp`, because they build
machine-node facts and publish effects from prepared call records.

## Watchouts

- Do not work on `ideas/open/03_dispatch_responsibility_reduction.md`.
- Do not broaden this checkpoint into whole dispatch cleanup or a whole
  calls-family rewrite.
- Do not claim helper renames, expectation rewrites, or testcase-shaped
  shortcuts as progress.
- Keep AArch64-specific register, memory, frame-slot, immediate, and assembly
  spelling in AArch64 code; the target of this checkpoint is ownership of
  printer and effect-publication responsibilities.
- Keep source-idea progress tied to deleted duplication, moved ownership, or a
  sharper emission-only boundary.
- Do not move `print_call_boundary_move` in the first Step 2 slice; it drags
  frame-slot load/address materialization, immediate materialization, scalar
  FPR, and f128 q-register spelling helpers, so it should follow after the
  smaller `print_call` boundary proves clean.
- Do not move `effect_from_prepared_call_preserved_value` to the printer.
  Blocker: the prepared preserved-value route is currently converted into
  machine-node `preserves` by `make_call_instruction`; there is no separate
  printer-owned fact that can replace that effect publication.

## Proof

No build or ctest was required for this read-only Step 1 mapping packet.

Intended focused Step 2 proof:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_(aarch64_machine_printer|aarch64_call_boundary_owner|call_boundary_effect_plan|prepared_printer|aarch64_instruction_dispatch)$'`
