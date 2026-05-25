Status: Active
Source Idea Path: ideas/open/02_aarch64_calls_emission_consolidation.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Broader Backend Checkpoint

# Current Packet

## Just Finished

Step 4 ran the broader backend checkpoint for the completed call
printing/effect-publication consolidation slices. The supervisor-selected
backend proof passed, confirming the completed consolidation work still holds
across the backend subset.

## Suggested Next

Supervisor should review the completed Step 4 checkpoint and decide whether to
send the active source idea to plan-owner for close, deactivation, or a follow-up
runbook decision.

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
- No Step 4 validation blocker remains for the active source idea after this
  backend checkpoint.

## Proof

Step 4 proof passed:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log 2>&1`

Proof log: `test_after.log`. The backend subset passed 162/162 tests.
