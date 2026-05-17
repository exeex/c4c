Status: Active
Source Idea Path: ideas/open/264_backend_and_aarch64_codegen_entrypoint_clarity.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Extract AArch64 Assembly Rendering Ownership

# Current Packet

## Just Finished

Step 2 extracted AArch64 `.s` text rendering ownership from
`src/backend/backend.cpp` into
`src/backend/mir/aarch64/codegen/asm_emitter.{hpp,cpp}`.

`backend.cpp` now prepares BIR and calls
`c4c::backend::aarch64::codegen::print_prepared_machine_nodes(...)` for the
AArch64 BIR and LIR entry routes. The helper owns the external assembler text
wrapper, still enters through
`c4c::backend::aarch64::codegen::compile_prepared_module(...)`, still uses
`mir::print_machine_function(...)` with `MachineInstructionPrinter`, and keeps
the existing assembly and error text behavior.

## Suggested Next

Step 3 should clarify the internal AArch64 compile coordinator naming and file
ownership around `codegen.hpp`, `emit.cpp`, and `emit.hpp`, while keeping
`compile_prepared_module(...)` as the public AArch64 entrypoint and preserving
codegen behavior.

## Watchouts

- `backend.cpp` still includes `<sstream>` for unrelated route-debug rendering.
- `asm_emitter.cpp` is now listed in `src/backend/CMakeLists.txt`; Step 3
  should account for that new AArch64 codegen source if it renames or splits
  compile-coordinator files.
- No instruction selection, lowering, printer line spelling, tests,
  `plan.md`, or source ideas were changed in Step 2.

## Proof

Supervisor-selected Step 2 proof passed and is preserved in `test_after.log`:

`{ cmake --build build --target c4cll backend_aarch64_signature_metadata_test backend_aarch64_machine_printer_test backend_aarch64_prepared_handoff_gate_test && ctest --test-dir build -R '^(backend_aarch64_signature_metadata|backend_aarch64_machine_printer|backend_aarch64_prepared_handoff_gate|backend_cli_aarch64_asm_external_return_zero_smoke|backend_cli_aarch64_asm_external_return_add_smoke|backend_cli_aarch64_asm_external_return_add_sub_chain_smoke)$' --output-on-failure; } > test_after.log 2>&1`
