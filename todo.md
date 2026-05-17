Status: Active
Source Idea Path: ideas/open/263_aarch64_codegen_public_compiled_module_interface.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Update tests, docs, and compatibility boundaries

# Current Packet

## Just Finished

Completed Step 4: Update tests, docs, and compatibility boundaries.

- Updated the signature metadata source-text audit to read
  `src/backend/mir/aarch64/codegen/codegen.hpp` directly and assert that the
  public compiled-module route exposes `codegen::CompiledModule`,
  `codegen::CompileResult`, and
  `codegen::compile_prepared_module(const PreparedBirModule&)`.
- Updated `src/backend/mir/aarch64/BIR_PREPARED_GAP_LEDGER.md` so
  `codegen/codegen.hpp` and the codegen-owned compiled result names are the
  public prepared-module contract.
- Preserved `module::Module` / `module::BuildResult` as internal target MIR
  snapshot names and documented `api::build_prepared_module(...)` as
  compatibility forwarding rather than the primary public route.

## Suggested Next

Recommended next packet: supervisor should review whether the Step 4 contract
update exhausts the active runbook, then route to plan-owner for lifecycle
closure, deactivation, or a follow-up split if needed.

## Watchouts

- `api::build_prepared_module(...)` intentionally remains in place as a
  compatibility wrapper, and the ledger now labels it that way.
- The signature metadata test uses source-text checks, so future public header
  formatting changes may need the test string updated with the API change.

## Proof

Ran the delegated proof exactly:

`{ cmake --build build --target c4c_backend backend_aarch64_signature_metadata_test backend_aarch64_prepared_handoff_gate_test backend_aarch64_module_skeleton_contract_test c4cll && ctest --test-dir build -R '^(backend_aarch64_signature_metadata|backend_aarch64_prepared_handoff_gate|backend_aarch64_module_skeleton_contract|backend_cli_aarch64_asm_external_return_zero_smoke)$' --output-on-failure; } > test_after.log 2>&1`

Result: passed. `test_after.log` shows the requested build targets completed
and all 4 selected tests passed:
`backend_aarch64_signature_metadata`,
`backend_aarch64_prepared_handoff_gate`,
`backend_aarch64_module_skeleton_contract`, and
`backend_cli_aarch64_asm_external_return_zero_smoke`.
