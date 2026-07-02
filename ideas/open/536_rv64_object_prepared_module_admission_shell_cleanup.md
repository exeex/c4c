# RV64 Object Prepared Module Admission Shell Cleanup

## Goal

Factor RV64 object-route prepared module admission and diagnostic construction into a thin behavior-preserving shell around the existing function conversion and data-object assembly.

## Why This Exists

`build_rv64_prepared_text_object_module_with_diagnostics` currently coordinates diagnostics, BIR function matching, object-function conversion, text module construction, and data-object emission. A narrow admission shell can reduce entrypoint coupling without moving high-risk object assembly or changing rejection behavior.

## In Scope

- Owned files:
  - `src/backend/mir/riscv/codegen/object_emission.cpp`
  - `src/backend/mir/riscv/codegen/object_emission.hpp`
  - `src/backend/mir/riscv/codegen/prepared_module_emit.cpp`
  - `src/backend/mir/riscv/codegen/prepared_module_emit.hpp`
- Factor prepared module admission and diagnostic construction only.
- Preserve `build_rv64_prepared_text_object_module_with_diagnostics` and `write_rv64_prepared_relocatable_elf_object_with_diagnostics` observable behavior.
- Keep parallel-copy diagnostics, prepared object consumer categories, and public wrapper behavior identical.

## Out Of Scope

- Moving `prepared_function_to_object_function`, `fragment_for_prepared_instruction`, text module assembly, prepared data-object assembly, relocation handling, or ELF writing.
- Inferring missing prepared facts from BIR, target text, object output, or testcase shape.
- Pass/fail accounting changes, gcc_torture expectation changes, unsupported marker changes, or RV64 capability repair.

## Acceptance Criteria

- Module admission has a clearer shell while preserving diagnostics and public entrypoint behavior.
- Function conversion and data-object assembly remain central unless a later idea owns them.
- Validation includes:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_(prepared_object_consumer_contract|riscv_object_emission|object_model_records|cli_riscv64_variadic_entry_missing_contract_obj|cli_riscv64_unsupported_global_diagnostic_obj)'`

## Reviewer Reject Signals

- The slice changes diagnostic categories, rejection text meaning, pass/fail accounting, or unsupported expectations.
- The implementation infers missing prepared facts or accepts formerly unsupported prepared-object states.
- The diff moves high-risk object assembly, function traversal, or data-object section emission.
- Helper renames or wrapper churn are claimed as capability progress.
- A new admission file hides the same monolithic function conversion and assembly coupling.
