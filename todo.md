Status: Active
Source Idea Path: ideas/open/570_rv64_unsupported_instruction_fragment_owner_diagnostics.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Diagnostic Emission Paths

# Current Packet

## Just Finished

Step 1 inventoried the current RV64 prepared object-route
`unsupported_instruction_fragment` emission paths without changing
implementation behavior.

- AST-backed queries used:
  `c4c-clang-tool-ccdb list-symbols` and `function-signatures` on
  `src/backend/mir/riscv/codegen/object_emission.cpp`,
  `function-callees` on
  `build_rv64_prepared_text_object_module_with_diagnostics`,
  `prepare_rv64_object_function_admission_shell`,
  `admit_rv64_prepared_module_function`,
  `diagnose_unsupported_prepared_instruction_fragment`, and
  `diagnose_unsupported_prepared_variadic_helper_fragment`, plus
  `function-callers` for
  `diagnose_unsupported_prepared_instruction_fragment`.
- Direct category sites found:
  `prepared_function_emit.cpp` rejects nonempty
  `function->atomic_operations` with
  `unsupported_instruction_fragment: atomic operations are not supported by the RV64 object route`;
  `object_emission.cpp` rejects a null prepared traversal instruction with
  `unsupported_instruction_fragment: prepared traversal instruction is missing`;
  both the complete prepared traversal path and fallback block/instruction loop
  reject unhandled failed instruction fragments with
  `unsupported_instruction_fragment: BIR instruction requires unsupported RV64 object lowering`.
- Contextual pre-fallback sites found:
  both instruction-fragment loops first call
  `diagnose_unsupported_prepared_variadic_helper_fragment`, then
  `diagnose_unsupported_prepared_instruction_fragment`. Those helpers already
  emit more specific categories for variadic helpers, local/global memory,
  floating casts, and scalar compare publications before the generic
  `unsupported_instruction_fragment` fallback is used.
- Propagation path:
  `prepared_function_to_object_function` returns
  `RiscvPreparedObjectFunctionResult`, then
  `build_rv64_prepared_text_object_module_with_diagnostics`, then
  `write_rv64_prepared_relocatable_elf_object_with_diagnostics`, then
  `emit_rv64_prepared_object_module`, where the backend object error appends
  the diagnostic text after
  `RISC-V backend object route unsupported prepared module shape`.
- First patch target:
  enrich the two generic `fragment_for_prepared_instruction` fallback sites in
  `prepared_function_to_object_function`, preferably through the existing
  `diagnose_unsupported_prepared_instruction_fragment` helper or a narrow
  wrapper around it so both traversal modes share the same owner-aware wording.

## Suggested Next

Implement the first diagnostic-only patch for the generic failed
`fragment_for_prepared_instruction` fallback so it reports the BIR instruction
owner/context while preserving the `unsupported_instruction_fragment` category.

## Watchouts

- This plan is diagnostic-only; do not implement unsupported RV64 lowering
  operations discovered by the improved diagnostics.
- Keep the `unsupported_instruction_fragment` category stable while enriching
  the diagnostic context.
- Do not add named-case matching for the nine representative source files.
- Do not change expectations, unsupported markers, allowlists, or runtime
  comparison behavior.
- The atomic-operation admission rejection is outside the instruction-fragment
  fallback and already has a distinct owner (`function->atomic_operations`);
  changing it should be a separate decision, not part of the first generic
  fragment fallback patch.
- The null prepared traversal instruction fallback has no BIR instruction value
  to inspect; surfacing more context there likely requires threading event or
  block metadata separately.
- `prepared_consumer_category` diagnostics bypass the plain string rejection
  path and should not be conflated with `unsupported_instruction_fragment`.

## Proof

Inventory-only `todo.md` update. Proof command:
`git diff --check -- todo.md`.
