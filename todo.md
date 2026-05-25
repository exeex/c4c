Status: Active
Source Idea Path: ideas/open/02_aarch64_calls_emission_consolidation.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Consolidate Effect Publication And Declarations

# Current Packet

## Just Finished

Step 3 started the calls API-surface cleanup after Step 2 removed the exported
print helpers. `calls.hpp` no longer includes `mir/printer.hpp`, and the stale
`// calls_printing` public section is now named `// calls_emission_nodes` to
match the remaining call-boundary construction and effect-publication factories.

`calls_printing.cpp` no longer includes `machine_printer.hpp`; it keeps only a
local declaration for `materialize_integer_constant_lines`, the remaining
non-printer helper dependency used by immediate cast publication.

## Suggested Next

Supervisor should review the Step 3 include/API-surface cleanup for acceptance
and decide whether remaining helper ownership, such as
`materialize_integer_constant_lines`, needs a separate neutral utility packet or
should stay as an implementation dependency.

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
- Do not move `effect_from_prepared_call_preserved_value` to the printer.
  Blocker: the prepared preserved-value route is currently converted into
  machine-node `preserves` by `make_call_instruction`; there is no separate
  printer-owned fact that can replace that effect publication.
- `calls_printing.cpp` still calls `materialize_integer_constant_lines`; this
  packet removed the printer-facing include without moving the helper because
  helper ownership was outside the delegated file set.

## Proof

Step 3 proof passed:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_(aarch64_machine_printer|aarch64_call_boundary_owner|call_boundary_effect_plan|prepared_printer|aarch64_instruction_dispatch)$' > test_after.log 2>&1`

Proof log: `test_after.log`. The focused subset passed 5/5 tests.
