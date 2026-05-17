Status: Active
Source Idea Path: ideas/open/264_backend_and_aarch64_codegen_entrypoint_clarity.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Backend And AArch64 Route Responsibilities

# Current Packet

## Just Finished

Step 1 audited `src/backend/backend.cpp` and the AArch64 codegen entry route.

`backend.cpp` helper classification:

- Generic driver: `profile_or_default`, `resolve_public_lir_target_profile`,
  `emit_bootstrap_lir_module`, `emit_target_bir_module`,
  `emit_target_lir_module`, `assemble_target_lir_module`, `emit_module`,
  `dump_module`, and `BackendModuleInput` constructors.
- Target route gates: `is_x86_target`, `is_aarch64_target`,
  `require_x86_module_entry_target`, and
  `require_aarch64_module_entry_target`.
- X86 route: `make_x86_lir_handoff_failure_message`,
  `emit_x86_bir_module_entry`, `emit_x86_lir_module_entry`, and
  `stage_x86_lir_module_entry`.
- AArch64 route: `make_aarch64_lir_handoff_failure_message`,
  `emit_aarch64_bir_module_entry`, and `emit_aarch64_lir_module_entry`.
- Shared BIR preparation/text fallback: `make_backend_dump_failure_message`,
  `render_prepared_bir_text`, `prepare_semantic_bir_pipeline`, and
  `prepare_bir_module_for_target`.
- Route-debug filtering: `dump_stage_uses_target_local_route_debug`,
  `dump_target_local_prepared_mir`, `dump_generic_semantic_bir`,
  `dump_generic_prepared_bir`, and the helper family from
  `count_matching_functions` through `filter_prepared_module_to_value`.
- AArch64 assembly rendering currently in `backend.cpp`:
  `print_aarch64_prepared_machine_nodes`.

Exact AArch64 machine-node-to-assembly rendering boundary to move in Step 2:
`print_aarch64_prepared_machine_nodes(...)` should leave `backend.cpp` as an
AArch64-owned helper. It should keep compiling with
`c4c::backend::aarch64::codegen::compile_prepared_module(prepared)`, then own
the `.text`, `.globl`, `.type`, label, `.size`, and `.note.GNU-stack`
assembly wrapping plus the walk over `built.module->functions` that invokes
`mir::print_machine_function(function.mir, MachineInstructionPrinter{})`.
The lower machine-node line spelling remains in
`codegen/machine_printer.cpp` through
`MachineInstructionPrinter::print_instruction(...)` and
`print_machine_instruction_line_payloads(...)`; the shared indentation and
machine-function walk remain in `mir::print_machine_function(...)`.

Public AArch64 callers can keep entering through
`aarch64::codegen::compile_prepared_module(...)`: `codegen.hpp` exposes
`CompiledModule`, `CompileResult`, and
`compile_prepared_module(const PreparedBirModule&)`; `emit.cpp` implements it
as the public wrapper over internal `build_module(...)`. Existing tests and
compatibility entry points already call this public symbol.

## Suggested Next

Step 2 should add an AArch64-owned assembly rendering helper under
`src/backend/mir/aarch64/` or `src/backend/mir/aarch64/codegen/`, move the body
of `print_aarch64_prepared_machine_nodes(...)` into it, and leave
`backend.cpp` responsible only for route selection, BIR preparation, and
calling that helper.

## Watchouts

- Keep the route behavior-preserving.
- Do not change emitted assembly, diagnostics, or test expectations.
- Do not route public callers around `c4c::backend::aarch64::codegen`.
- Preserve the current error messages from the moved helper unless Step 2 also
  updates focused route contracts intentionally.
- Step 2 should not move or duplicate `mir::print_machine_function(...)`; only
  the AArch64 compiled-module-to-external-assembler text wrapper belongs in the
  AArch64 route.

## Proof

Audit-only packet; no build required and no `test_after.log` produced. Focused
Step 2 proof subset should be:

`cmake --build build --target c4c backend_aarch64_signature_metadata_test backend_aarch64_machine_printer_test backend_aarch64_prepared_handoff_gate_test`

then:

`ctest --test-dir build -R '^(backend_aarch64_signature_metadata|backend_aarch64_machine_printer|backend_aarch64_prepared_handoff_gate|backend_cli_aarch64_asm_external_return_zero_smoke|backend_cli_aarch64_asm_external_return_add_smoke|backend_cli_aarch64_asm_external_return_add_sub_chain_smoke)$' --output-on-failure`
