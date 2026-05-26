Status: Active
Source Idea Path: ideas/open/38_aarch64_module_compatibility_foldback_cleanup.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Compatibility Projection Ownership

# Current Packet

## Just Finished

Step 1 - Inventory Compatibility Projection Ownership completed the
no-implementation inventory packet:

- `compatibility_projection.hpp` exposes only two declarations:
  `derive_compatibility_function_records` and
  `derive_compatibility_projection`.
- `compatibility_projection.cpp` builds legacy `module::FunctionRecord` views
  from `module::MachineFunction` values by copying the function id, prepared
  label, MIR function, and `module::selected_machine_nodes(function)` into
  `FunctionRecord::machine_nodes`; it then wraps those records in
  `module::CompatibilityProjection`.
- The only live include of `compatibility_projection.hpp` outside the helper
  file itself is `module_compile.cpp`.
- The only direct callers are in `module_compile.cpp` inside
  `lower_module_body`, which assigns `built_module.functions` and
  `built_module.compatibility` after `lower_prepared_functions`.
- Legacy projection readers are tests that inspect `module.functions`,
  `module.compatibility.functions`, and/or `FunctionRecord::machine_nodes`,
  especially `backend_aarch64_module_skeleton_contract_test.cpp`,
  `backend_aarch64_function_traversal_test.cpp`,
  `backend_aarch64_return_lowering_test.cpp`,
  `backend_aarch64_branch_control_lowering_test.cpp`, and
  `backend_aarch64_prepared_scalar_alu_records_test.cpp`.
- `module::FunctionRecord`, `module::CompatibilityProjection`, and
  `module::Module::functions` remain declared in
  `src/backend/mir/aarch64/module/module.hpp`; the selected-node projection
  helper `module::selected_machine_nodes` remains in `module/module.cpp`.
- Build metadata contains
  `mir/aarch64/codegen/compatibility_projection.cpp` in
  `src/backend/CMakeLists.txt`.
- No compatibility projection declarations need to remain public after
  fold-back if implementation moves into `module_compile.cpp`; both helper
  functions can be namespace-local there. The resulting projection records
  still need to be produced for existing legacy readers unless a separate
  behavior-changing cleanup removes those consumers.
- `backend_aarch64_signature_metadata_test.cpp` contains path metadata for
  `compatibility_projection.cpp/.hpp`; if the files are deleted, that test may
  need a narrow compile/test metadata update as include/build fallout.

## Suggested Next

Recommended first mechanical fold-back packet: move
`derive_compatibility_function_records` and
`derive_compatibility_projection` into the anonymous namespace in
`module_compile.cpp`, remove the `compatibility_projection.hpp` include, delete
`compatibility_projection.cpp/.hpp`, and remove
`compatibility_projection.cpp` from `src/backend/CMakeLists.txt`. Keep
`built_module.functions` and `built_module.compatibility` populated exactly as
they are today.

## Watchouts

Keep this route mechanical. Do not make `FunctionRecord::machine_nodes` a
terminal assembly authority, route new behavior through compatibility
projection records, rewrite final assembly emission, alter MIR stream
ownership, or mix in calls/dispatch/memory/comparison/prologue cleanup. The
next packet should preserve projection construction for current legacy readers;
deleting the behavior outright would not be mechanical.

## Proof

No build required for inventory-only lifecycle update.

`git diff --check -- todo.md`
