Status: Active
Source Idea Path: ideas/open/aarch64-codegen-02-compatibility-projection-bridge-retirement.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Focused Proof

# Current Packet

## Just Finished

Step 3 - Focused Proof proved the compatibility projection contraction
preserves behavior. Readback confirmed the only code-changing slice moved the
selected non-return machine-node filtering rule from
`compatibility_projection.cpp` to `module::selected_machine_nodes`; no
implementation or test expectation changes were made in this packet. Full
`FunctionRecord` and `CompatibilityProjection` behavior remains intact because
`derive_compatibility_function_records` still owns the compatibility wrapper,
keeps the prepared-name `label`, preserves the full `mir`, and only delegates
the flat selected-node view to the module layer.

## Suggested Next

Hand this runbook to the plan owner for closure readiness review. The executed
scope is complete: the selected-machine-node filter bridge has been localized,
backend proof is green, and no testcase-overfit or expectation downgrade was
introduced.

## Watchouts

- The selected-node filter is now module-owned, but compatibility wrappers
  remain behaviorally intact. Do not remove `FunctionRecord::label` or full
  `CompatibilityProjection` until a separate gate proves those facts have a
  direct owner.
- Object/global records are not sourced from compatibility projection:
  `ModuleDataRecords` is empty and assembly globals still print from
  `prepared.module`.
- Unsupported-node reports and diagnostics are not preserved by the projection;
  `module_compile.cpp` currently builds local `ModuleLoweringDiagnostics` and
  does not store them on `module::Module`, while tests assert unsupported paths
  through direct dispatch diagnostics or empty selected-node projections.
- Remaining bridge caveat: this runbook only contracted the selected
  non-return machine-node filtering path. Broader `FunctionRecord`,
  `CompatibilityProjection`, object/global, and diagnostic ownership changes
  need separate gates.

## Proof

`bash -lc 'set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"; } 2>&1 | tee test_after.log'`
passed. The backend subset ran 162 tests with 162 passed and 0 failed. The
proof covers the AArch64 compatibility-adjacent backend tests, selected target
records, unsupported-node reporting paths, object/global-adjacent backend
coverage, and the broader backend route set selected by the supervisor.
`git diff --check` passed. No tests or expectations were changed for this
packet, so there is no expectation downgrade. Proof log: `test_after.log`.
