Status: Active
Source Idea Path: ideas/open/aarch64-codegen-02-compatibility-projection-bridge-retirement.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Contract or Localize the Proven Bridge Path

# Current Packet

## Just Finished

Step 2 - Contract or Localize the Proven Bridge Path moved the selected
non-return machine-node filtering helper out of `compatibility_projection.cpp`
and into the AArch64 module layer as `module::selected_machine_nodes`.
`derive_compatibility_function_records` still builds the full compatibility
`FunctionRecord` wrapper, including the prepared-name `label`, but now consumes
the module-owned selected-node view instead of owning the filtering rule itself.

## Suggested Next

Execute Step 3 by retiring or redirecting the narrowest remaining compatibility
consumer that only needs selected non-return target records to call
`module::selected_machine_nodes` or walk `module::MachineFunction` directly.
Leave `FunctionRecord::label`, full `FunctionRecord` wrapping, object/global
projection, diagnostics, and broader `CompatibilityProjection` retirement out
of scope.

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
- Step 3 should target a real compatibility consumer, not rewrite tests or
  weaken expectations.

## Proof

`bash -lc 'set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(backend_aarch64_function_traversal|backend_aarch64_return_lowering|backend_aarch64_branch_control_lowering|backend_aarch64_module_skeleton_contract|backend_aarch64_instruction_dispatch)$"; } 2>&1 | tee test_after.log'` passed. The selected subset covered `backend_aarch64_function_traversal`, `backend_aarch64_return_lowering`, `backend_aarch64_branch_control_lowering`, `backend_aarch64_module_skeleton_contract`, and `backend_aarch64_instruction_dispatch`; all 5 tests passed. `git diff --check` passed. Proof log: `test_after.log`.
