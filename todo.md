Status: Active
Source Idea Path: ideas/open/536_rv64_object_prepared_module_admission_shell_cleanup.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map Admission And Diagnostic Boundaries

# Current Packet

## Just Finished

Step 1 - Map Admission And Diagnostic Boundaries: mapped the prepared object
module admission and diagnostic boundary around
`build_rv64_prepared_text_object_module_with_diagnostics` and
`write_rv64_prepared_relocatable_elf_object_with_diagnostics`.

Safe Step 2 extraction boundary:

- Move or introduce only the prepared-module admission/diagnostic helpers needed
  to keep the object-route entrypoint thin:
  `make_rv64_prepared_module_rejection`,
  `make_rv64_prepared_image_rejection`,
  `diagnose_unplaced_parallel_copy_obligations`, and a small admission result
  helper for each prepared control-flow function that preserves the current
  behavior for unplaced parallel-copy obligations, missing prepared target
  names, declaration-only BIR functions, prepared-consumer diagnostics, failed
  object-function conversion diagnostics, empty function sets, module-build
  failure diagnostics, data-object diagnostics, and image-level diagnostic
  forwarding.
- Keep the moved admission helper independent of object-function conversion and
  final object assembly. It may report "emit this prepared function" or "skip
  this declaration-only function", but it should not lower fragments, append
  data objects, write relocations, or serialize ELF.
- Preserve current public wrappers and observable return behavior for
  `build_rv64_prepared_text_object_module_with_diagnostics`,
  `build_rv64_prepared_text_object_module`,
  `write_rv64_prepared_relocatable_elf_object_with_diagnostics`, and
  `write_rv64_prepared_relocatable_elf_object`.

Explicit parked non-move set for Step 2:

- Do not move `prepared_function_to_object_function`.
- Do not move `fragment_for_prepared_instruction` or any instruction fragment
  lowering helpers.
- Do not move `build_rv64_text_object_module`.
- Do not move `append_rv64_prepared_data_objects`,
  `rv64_prepared_object_data_symbol_binding`, relocation symbol lookup, global
  data section emission, or prepared data-object assembly.
- Do not move `write_rv64_relocatable_elf_object` or ELF writer/configuration
  handling.
- Leave current conversion-side use of `find_defined_bir_function` parked in
  `object_emission.cpp`; if Step 2 needs admission-side declaration detection
  in `prepared_module_emit.*`, keep it as narrow admission logic and avoid
  dragging conversion helpers across the boundary.

Current consumers and header prerequisites:

- Public prepared object diagnostics are consumed through `object_emission.hpp`
  by `src/backend/backend.cpp` and object-emission tests, so public wrapper
  signatures should remain source-compatible.
- `prepared_module_emit.hpp` currently exposes only `emit_prepared_module_text`;
  Step 2 may need a narrow admission result type/helper declaration there, with
  includes limited to prepared-module and prepared-object diagnostic types.
- `object_emission.cpp` can include `prepared_module_emit.hpp` for the new
  admission helper while keeping object result structs and public object-route
  wrappers in `object_emission.hpp`.

## Suggested Next

Execute Step 2 from `plan.md`: extract the prepared module admission shell into
`prepared_module_emit.*` using the safe boundary above, keep public wrappers
behavior-compatible, and leave conversion, text module assembly, prepared
data-object assembly, relocation handling, and ELF writing parked.

## Watchouts

- Preserve prepared-object rejection behavior, diagnostic category/text meaning,
  unsupported markers, expectations, pass/fail accounting, and RV64 capability
  support.
- Do not claim progress by moving the whole monolithic conversion/build loop
  behind `prepared_module_emit.*`; the source idea only owns admission and
  diagnostic construction.
- Keep parallel-copy diagnostics in the prepared-object consumer category path,
  and keep generic admission/module/image failures as plain diagnostic strings.

## Proof

No build run, per mapping-only packet.

Step 2 validation command:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_(prepared_object_consumer_contract|riscv_object_emission|object_model_records|cli_riscv64_variadic_entry_missing_contract_obj|cli_riscv64_unsupported_global_diagnostic_obj)'
```
