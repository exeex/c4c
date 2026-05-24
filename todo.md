Status: Active
Source Idea Path: ideas/open/aarch64-codegen-02-compatibility-projection-bridge-retirement.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Ownership Gate

# Current Packet

## Just Finished

Step 1 - Ownership Gate inspected the narrow `compatibility_projection.*`
selected-machine-node filtering bridge path. `derive_compatibility_function_records`
builds `FunctionRecord::machine_nodes` by walking `module::MachineFunction`
blocks/instructions and copying only non-return target records whose
`selection.status` is `Selected`; each projected node fact is already owned by
the selected `MachineInstruction::target` record in `module::MachineFunction`.
The proof clears this narrow path for Step 2 retirement work. Full function
record wrapping is not cleared by this gate because `FunctionRecord::label` is
still projected from `prepared.names`, outside the selected target records and
outside `module::MachineFunction`.

## Suggested Next

Execute Step 2 for the selected-machine-node filtering bridge only: replace
one narrow legacy `module.functions.front().machine_nodes` or
`module.compatibility.functions.front().machine_nodes` consumer with a direct
walk of `module.mir.functions`/`module::MachineFunction` selected non-return
target records, leaving full `FunctionRecord`/`CompatibilityProjection`
retirement out of scope.

## Watchouts

- The ownership proof only covers the flat selected-machine-node filtering
  path. It does not clear full function record wrapping because the projected
  `label` fact is still derived from prepared names.
- Object/global records are not sourced from compatibility projection:
  `ModuleDataRecords` is empty and assembly globals still print from
  `prepared.module`.
- Unsupported-node reports and diagnostics are not preserved by the projection;
  `module_compile.cpp` currently builds local `ModuleLoweringDiagnostics` and
  does not store them on `module::Module`, while tests assert unsupported paths
  through direct dispatch diagnostics or empty selected-node projections.
- Keep Step 2 scoped to deriving the flat selected non-return list from MIR.
  Do not treat this gate as permission to remove `FunctionRecord::label`,
  `Module::functions`, or `CompatibilityProjection` wholesale.

## Proof

`git diff --check` passed. No build and no `test_after.log` were required by
this no-code ownership gate.
