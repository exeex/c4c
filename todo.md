Status: Active
Source Idea Path: ideas/open/264_backend_and_aarch64_codegen_entrypoint_clarity.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Clarify The Internal AArch64 Compile Coordinator

# Current Packet

## Just Finished

Step 3 clarified the internal AArch64 prepared-module compile coordinator by
renaming `src/backend/mir/aarch64/codegen/emit.{hpp,cpp}` to
`src/backend/mir/aarch64/codegen/module_compile.{hpp,cpp}`.

`codegen.hpp` remains the public API surface for
`c4c::backend::aarch64::codegen::compile_prepared_module(...)`, while
`module_compile.hpp` now names the internal `build_module(...)` coordinator.
`module.cpp`, `src/backend/CMakeLists.txt`, and the signature metadata source
inventory now include the clarified internal filename/header name.

## Suggested Next

Step 4 should align AArch64 route documentation and contract text with the
clarified ownership model: public entry in `codegen.hpp`, internal coordinator
in `module_compile.{hpp,cpp}`, and downstream rendering/assembler ownership in
the narrower helpers.

## Watchouts

- Some AArch64 docs still mention the old `codegen/emit.hpp` name and should
  be handled by Step 4, not this code-only rename packet.
- No instruction selection, lowering, printer line spelling, `backend.cpp`,
  `plan.md`, or source ideas were changed in Step 3.

## Proof

Supervisor-selected Step 3 proof passed and is preserved in `test_after.log`:

`{ cmake --build build --target c4cll backend_aarch64_signature_metadata_test backend_aarch64_prepared_handoff_gate_test backend_aarch64_module_skeleton_contract_test backend_aarch64_function_traversal_test && ctest --test-dir build -R '^(backend_aarch64_signature_metadata|backend_aarch64_prepared_handoff_gate|backend_aarch64_module_skeleton_contract|backend_aarch64_function_traversal)$' --output-on-failure; } > test_after.log 2>&1`
