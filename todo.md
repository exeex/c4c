Status: Active
Source Idea Path: ideas/open/263_aarch64_codegen_public_compiled_module_interface.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Migrate API-facing and backend callers

# Current Packet

## Just Finished

Completed Step 3: Migrate API-facing and backend callers.

- Updated `src/backend/backend.cpp` to include the public AArch64 codegen
  header and call
  `c4c::backend::aarch64::codegen::compile_prepared_module(...)` for the
  prepared-machine-node assembly route.
- Migrated the listed public-route AArch64 backend tests from
  `aarch64_api::build_prepared_module(...)` to
  `aarch64_codegen::compile_prepared_module(...)`.
- Left target MIR record assertions and internal module namespaces in place;
  this was a behavior-preserving call-site migration only.

## Suggested Next

Recommended next packet: migrate any remaining non-public or internal
compatibility callers only if the active plan's next step explicitly calls for
retiring the legacy API wrapper; otherwise move to the next public-interface
contract step.

## Watchouts

- The legacy `aarch64::api::build_prepared_module(...)` wrapper still exists as
  a compatibility surface; this packet only moved the delegated API-facing and
  backend callers.
- Tests still assert target MIR records through `aarch64::module`, as requested.

## Proof

Ran the delegated proof exactly:

`{ cmake --build build --target c4c_backend backend_aarch64_prepared_handoff_gate_test backend_aarch64_module_skeleton_contract_test backend_aarch64_function_traversal_test backend_aarch64_return_lowering_test backend_aarch64_branch_control_lowering_test backend_aarch64_instruction_dispatch_test backend_aarch64_prepared_scalar_alu_records_test c4cll && ctest --test-dir build -R '^(backend_aarch64_prepared_handoff_gate|backend_aarch64_module_skeleton_contract|backend_aarch64_function_traversal|backend_aarch64_return_lowering|backend_aarch64_branch_control_lowering|backend_aarch64_instruction_dispatch|backend_aarch64_prepared_scalar_alu_records|backend_cli_aarch64_asm_external_return_zero_smoke)$' --output-on-failure; } > test_after.log 2>&1`

Result: passed. `test_after.log` shows the requested build targets completed
and all 8 selected tests passed:
`backend_aarch64_prepared_handoff_gate`,
`backend_aarch64_module_skeleton_contract`,
`backend_aarch64_function_traversal`,
`backend_aarch64_return_lowering`,
`backend_aarch64_branch_control_lowering`,
`backend_aarch64_instruction_dispatch`,
`backend_aarch64_prepared_scalar_alu_records`, and
`backend_cli_aarch64_asm_external_return_zero_smoke`.
