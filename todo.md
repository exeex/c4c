Status: Active
Source Idea Path: ideas/open/536_rv64_object_prepared_module_admission_shell_cleanup.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Extract The Prepared Module Admission Shell

# Current Packet

## Just Finished

Step 2 - Extract The Prepared Module Admission Shell: moved the narrow
prepared-module admission and diagnostic shell into `prepared_module_emit.*`.
The moved shell now owns `make_rv64_prepared_module_rejection`,
`make_rv64_prepared_image_rejection`,
`diagnose_unplaced_parallel_copy_obligations`, admission-side defined-function
detection, and `admit_rv64_prepared_module_function`.

The extracted admission helper preserves the existing behavior for unplaced
parallel-copy prepared-consumer diagnostics, missing prepared target names, and
declaration-only BIR functions. `object_emission.cpp` still owns public
object-route wrappers plus object-function conversion, text module assembly,
prepared data-object assembly, relocation handling, and ELF serialization.

Public signatures and observable result behavior for
`build_rv64_prepared_text_object_module_with_diagnostics`,
`build_rv64_prepared_text_object_module`,
`write_rv64_prepared_relocatable_elf_object_with_diagnostics`, and
`write_rv64_prepared_relocatable_elf_object` remain source-compatible.

## Suggested Next

Execute Step 3 from `plan.md`: inspect the remaining object-route wrappers after
the admission extraction. The concrete cleanup target is to decide whether the
newly public `make_rv64_prepared_module_rejection`,
`make_rv64_prepared_image_rejection`, and
`admit_rv64_prepared_module_function` declarations should stay as the intended
prepared-module boundary, and to delete only genuinely dead forwarding wrappers
if any exist.

## Watchouts

- `prepared_function_to_object_function`, `fragment_for_prepared_instruction`,
  `build_rv64_text_object_module`, `append_rv64_prepared_data_objects`,
  relocation handling, and ELF writing are intentionally still parked in
  `object_emission.cpp`.
- Keep parallel-copy diagnostics in the prepared-object consumer category path,
  and keep generic admission/module/image failures as plain diagnostic strings.
- `object_emission.hpp` did not need to change for Step 2; public prepared
  object result structs and public entrypoints remain there.

## Proof

Ran the delegated Step 2 proof command. Result: passed, 5/5 targeted tests
green. Proof log: `test_after.log`.

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_(prepared_object_consumer_contract|riscv_object_emission|object_model_records|cli_riscv64_variadic_entry_missing_contract_obj|cli_riscv64_unsupported_global_diagnostic_obj)'
```
