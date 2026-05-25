Status: Active
Source Idea Path: ideas/open/02_aarch64_calls_emission_consolidation.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Move Or Narrow Pure Printer Ownership

# Current Packet

## Just Finished

Step 2 moved the pure `print_call` assembly spelling helper into
`machine_printer.cpp` at the `CallInstructionRecord` payload dispatch boundary.
`calls.hpp` no longer exports `print_call`, and `calls_printing.cpp` no longer
implements it.

`make_call_instruction`, variadic call record completion/selection, and
preserved-value effect publication remain calls-owned in `calls_printing.cpp`.

## Suggested Next

Continue Step 2 by deciding whether `print_call_boundary_abi_binding` is ready
to move to `machine_printer.cpp`, or should first be narrowed/deleted if the
binding node remains diagnostic-only.

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
- Do not move `print_call_boundary_move` in the next Step 2 slice; it drags
  frame-slot load/address materialization, immediate materialization, scalar
  FPR, and f128 q-register spelling helpers, so it should follow after the
  smaller `print_call` boundary proves clean.
- Do not move `effect_from_prepared_call_preserved_value` to the printer.
  Blocker: the prepared preserved-value route is currently converted into
  machine-node `preserves` by `make_call_instruction`; there is no separate
  printer-owned fact that can replace that effect publication.

## Proof

Step 2 proof passed:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_(aarch64_machine_printer|aarch64_call_boundary_owner|call_boundary_effect_plan|prepared_printer|aarch64_instruction_dispatch)$'`

Proof log accepted and rolled forward to canonical `test_before.log`. The
focused subset passed 5/5 tests.
