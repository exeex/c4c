Status: Active
Source Idea Path: ideas/open/263_aarch64_codegen_public_compiled_module_interface.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add the public codegen header and names

# Current Packet

## Just Finished

Completed Step 2: Add the public codegen header and names.

- Added `src/backend/mir/aarch64/codegen/codegen.hpp` as the public compiled
  module surface for AArch64 codegen.
- Published `c4c::backend::aarch64::codegen::CompiledModule` as an alias for
  `module::Module` and `codegen::CompileResult` as an alias for
  `module::BuildResult`.
- Declared and implemented `codegen::compile_prepared_module(...)` as a
  behavior-preserving wrapper around the existing `codegen::build_module(...)`
  coordinator.
- Updated `api::build_prepared_module(...)` to include the public codegen
  header, return `codegen::CompileResult`, and forward through
  `codegen::compile_prepared_module(...)`.
- Left target MIR records, diagnostics, and lowerer internals in
  `aarch64::module`; no lowering behavior or assembly behavior changed.

## Suggested Next

Recommended next packet: update the narrow public-contract/signature coverage
to recognize `codegen/codegen.hpp`, `codegen::CompileResult`, and
`codegen::compile_prepared_module(...)` if the active plan's next step calls
for test or metadata coverage of the new public surface.

## Watchouts

- `module::build(...)` still forwards to the internal `codegen::build_module(...)`
  compatibility coordinator; that was intentionally left unchanged for this
  behavior-preserving public-header slice.
- `codegen::CompiledModule` and `codegen::CompileResult` are aliases, not moved
  owning types, so existing callers observing `.module`, `.error`, and target
  MIR records remain source-compatible.
- No tests were changed in this packet.

## Proof

Ran the delegated proof exactly:

`{ cmake --build build --target c4c_backend backend_aarch64_prepared_handoff_gate_test backend_aarch64_module_skeleton_contract_test backend_aarch64_function_traversal_test c4cll && ctest --test-dir build -R '^(backend_aarch64_prepared_handoff_gate|backend_aarch64_module_skeleton_contract|backend_aarch64_function_traversal|backend_cli_aarch64_asm_external_return_zero_smoke)$' --output-on-failure; } > test_after.log 2>&1`

Result: passed. `test_after.log` shows the requested build targets completed
and all 4 selected tests passed:
`backend_aarch64_prepared_handoff_gate`,
`backend_aarch64_module_skeleton_contract`,
`backend_aarch64_function_traversal`, and
`backend_cli_aarch64_asm_external_return_zero_smoke`.
