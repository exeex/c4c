Status: Active
Source Idea Path: ideas/open/263_aarch64_codegen_public_compiled_module_interface.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add the public codegen header and names

# Current Packet

## Just Finished

Completed Step 1 audit for
`ideas/open/263_aarch64_codegen_public_compiled_module_interface.md`.

Current compiled-module public surface:
- `src/backend/mir/aarch64/module/module.hpp` defines
  `c4c::backend::aarch64::module::Module` at the prepared-module target MIR
  boundary. It carries the retained `PreparedBirModule*`, resolved
  `TargetProfile`, `MachineModule`, empty `ModuleDataRecords`,
  `CompatibilityProjection`, and compatibility `FunctionRecord` vector.
- `src/backend/mir/aarch64/module/module.hpp` defines
  `c4c::backend::aarch64::module::BuildResult` as
  `std::optional<Module> module` plus `std::optional<abi::HandoffError> error`.
- `src/backend/mir/aarch64/module/module.hpp` declares
  `module::build(const prepare::PreparedBirModule&)`; `module/module.cpp`
  forwards it to `codegen::build_module(prepared)`.
- `src/backend/mir/aarch64/codegen/emit.hpp` currently advertises the
  codegen coordinator, but it returns `module::BuildResult` from
  `codegen::build_module(...)`, so the public result type is still
  module-owned.
- `src/backend/mir/aarch64/api/api.hpp` includes `../module/module.hpp` and
  exposes `api::build_prepared_module(...)` returning
  `c4c::backend::aarch64::module::BuildResult`.

Public callers/includes that advertise the module-owned handoff:
- `src/backend/backend.cpp` includes the AArch64 API and consumes
  `aarch64::api::build_prepared_module(...)`; it observes `built.module`,
  `built.error`, and `built.module->functions` for the public asm route.
- Public API-facing tests include `src/backend/mir/aarch64/api/api.hpp` and call
  `aarch64_api::build_prepared_module(...)` in
  `backend_aarch64_prepared_handoff_gate_test.cpp`,
  `backend_aarch64_module_skeleton_contract_test.cpp`,
  `backend_aarch64_function_traversal_test.cpp`,
  `backend_aarch64_return_lowering_test.cpp`,
  `backend_aarch64_branch_control_lowering_test.cpp`,
  `backend_aarch64_instruction_dispatch_test.cpp`, and
  `backend_aarch64_prepared_scalar_alu_records_test.cpp`.
- Direct module-header tests include
  `src/backend/mir/aarch64/module/module.hpp` and alias
  `c4c::backend::aarch64::module` in the skeleton, MIR carrier, traversal,
  operand, branch, return, and instruction-dispatch coverage; most of those
  direct uses exercise diagnostics/records rather than the public compiled
  result contract.
- Source-text audit `backend_aarch64_signature_metadata_test.cpp` names
  `src/backend/mir/aarch64/api/api.*` and `module/module.*`, so it may need a
  narrow expectation update after the public header appears.

Internal implementation surfaces that can remain in `module` for now:
- Target MIR snapshots and carriers in `module.hpp`: `FunctionRecord`,
  `CompatibilityProjection`, `ModuleDataRecords`, machine aliases,
  lowering contexts, lowering diagnostic kinds/entries, and diagnostics
  collections.
- Codegen lowerers that take `module::ModuleLoweringDiagnostics&` or include
  `../module/module.hpp` for target MIR records, including `alu`, `calls`,
  `cast_ops`, `comparison`, `dispatch`, `globals`, `memory`, `operands`,
  `returns`, `traversal`, and `compatibility_projection`.
- `codegen::build_module(...)` can remain the internal coordinator name during
  the first slice if `codegen.hpp` publishes the new public spelling as a thin
  wrapper.

## Suggested Next

Execute Step 2 with the smallest behavior-preserving public-interface slice:
- add `src/backend/mir/aarch64/codegen/codegen.hpp`;
- in `namespace c4c::backend::aarch64::codegen`, publish
  `using CompiledModule = module::Module;` and
  `using CompileResult = module::BuildResult;`;
- add `[[nodiscard]] CompileResult compile_prepared_module(const
  c4c::backend::prepare::PreparedBirModule& prepared);`;
- implement the wrapper by forwarding to the existing internal
  `codegen::build_module(prepared)`;
- change `src/backend/mir/aarch64/api/api.hpp` to include `../codegen/codegen.hpp`
  and return `codegen::CompileResult` from `build_prepared_module(...)`;
- change `src/backend/mir/aarch64/api/api.cpp` to forward to
  `codegen::compile_prepared_module(...)`;
- do not move target MIR records or diagnostics out of `module.hpp` in this
  first slice.

Recommended focused proof for that first code slice:
`cmake --build build --target c4c_backend backend_aarch64_prepared_handoff_gate_test backend_aarch64_module_skeleton_contract_test backend_aarch64_function_traversal_test c4cll && ctest --test-dir build -R '^(backend_aarch64_prepared_handoff_gate|backend_aarch64_module_skeleton_contract|backend_aarch64_function_traversal|backend_cli_aarch64_asm_external_return_zero_smoke)$' --output-on-failure`

## Watchouts

- Keep this behavior-preserving; no lowering, emitted assembly, or diagnostics
  changes.
- Do not broaden into a full namespace purge of lower-level module,
  diagnostic, or lowering internals.
- Compatibility wrappers are allowed only as staged migration support; the
  public route should advertise `aarch64::codegen` names.
- `codegen/emit.hpp` currently includes `../module/module.hpp`; the new public
  `codegen.hpp` may also need that include for aliases, but avoid creating an
  include cycle through API headers.
- Direct module-header tests mostly validate internal target MIR/diagnostic
  records and need not migrate in the first packet unless they explicitly test
  the compiled result contract.
- Include public asm smoke in proof once code changes begin; the external smoke
  test may skip if the host lacks clang or an AArch64 assembler, but it is the
  focused public route to keep in the command.

## Proof

Audit-only metadata update. Per packet instructions, no build, CTest, or root
proof-log update was run.
