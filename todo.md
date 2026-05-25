Status: Active
Source Idea Path: ideas/open/02_aarch64_calls_emission_consolidation.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Consolidate Effect Publication And Declarations

# Current Packet

## Just Finished

Step 3 moved `materialize_integer_constant_lines` out of the machine-printer
API into neutral AArch64 codegen helper files,
`constant_materialization.hpp` and `constant_materialization.cpp`.
`machine_printer.hpp` no longer declares the helper, current materialization
users include the neutral header directly, and `calls_printing.cpp` no longer
keeps a private redeclaration.

## Suggested Next

Supervisor should review the Step 3 helper-ownership cleanup for acceptance and
decide whether the remaining effect-publication/API boundary is complete for
this checkpoint or needs another narrow packet.

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
- `materialize_integer_constant_lines` remains AArch64-codegen-owned, not
  printer-owned. Keep any follow-up users pointed at
  `constant_materialization.hpp` instead of `machine_printer.hpp`.

## Proof

Step 3 proof passed:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log 2>&1`

Proof log: `test_after.log`. The backend subset passed 162/162 tests.
