Status: Active
Source Idea Path: ideas/open/543_rv64_object_data_symbol_fixup_module_cleanup.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map Late Object Assembly Ownership

# Current Packet

## Just Finished

Step 1 mapped the late RV64 object assembly ownership without implementation
changes.

Current ownership map:
- Public object entrypoints in `object_emission.hpp` are still
  `build_rv64_text_object_module`,
  `build_rv64_prepared_text_object_module`,
  `build_rv64_prepared_text_object_module_with_diagnostics`,
  `write_rv64_relocatable_elf_object`,
  `write_rv64_prepared_relocatable_elf_object_with_diagnostics`, and
  `write_rv64_prepared_relocatable_elf_object`.
- Text section creation, function byte append order, local label binding,
  function symbol definition, text fixup symbol lookup/declaration, relocation
  kind mapping, and text relocation attachment are owned together by
  `build_rv64_text_object_module`.
- Data object emission is owned by `append_rv64_prepared_data_objects`: string
  constants go to `.rodata`; prepared globals are admitted through selected
  object-data contract facts, then emitted to `.bss`, `.rodata`, or `.data`.
- Zero-fill reservation is local to `append_rv64_prepared_data_objects` through
  selected zero-fill classification and `reserve_section_bytes`; it must stay
  tied to object-data identity, size, alignment, and BSS section selection.
- Section selection and section flags are currently centralized in
  `append_rv64_prepared_data_objects` for data and in
  `build_rv64_text_object_module` for `.text`; final section ordering is
  therefore a product of text module creation followed by data append.
- Symbol binding is split by kind: function binding uses
  `binding_for_function`, data binding uses
  `rv64_prepared_object_data_symbol_binding`, string constants are local
  object symbols, and unresolved fixup targets are declared as global
  undefined symbols.
- Data pointer relocation handling is owned by
  `append_rv64_prepared_data_objects` through
  `rv64_selected_symbol_pointer_initializer_label`,
  `rv64_find_or_declare_relocation_symbol`, `rv64_prepared_link_symbol_kind`,
  and `attach_relocation` with `kRiscvReloc64`.
- ELF config and serialization stay thin and public:
  `rv64_relocatable_elf_config` fixes RV64 little-endian flags, and
  `write_rv64_relocatable_elf_object` delegates to the object writer.
- Final prepared module/image assembly is owned by
  `build_rv64_prepared_text_object_module_with_diagnostics` and
  `write_rv64_prepared_relocatable_elf_object_with_diagnostics`, which sequence
  function admission, function emission, text module construction, data append,
  and ELF image writing.

First executable sub-slice for Step 2: review
`append_rv64_prepared_data_objects` for a narrow data-emission helper boundary
that keeps selected object-data facts, zero-fill classification, pointer
relocation creation, section selection, and symbol definition explicit. If that
requires a broad context or merges text fixups with data pointer relocations,
Step 2 should be recorded as no-code.

## Suggested Next

Execute Step 2 by evaluating the `append_rv64_prepared_data_objects` boundary
only; do not move text fixup attachment or final ELF/module assembly in that
packet.

## Watchouts

- Preserve object bytes, relocations, symbol bindings, section layout, ELF
  flags, public entrypoints, and zero-fill behavior.
- Keep text fixup attachment separate from data-object pointer relocation
  handling unless a stable shared contract already exists.
- A Step 2 extraction is only useful if the helper inputs/outputs are narrower
  than the current central function and do not hide section/symbol/relocation
  state behind a catch-all context.
- Do not touch tests, expectations, unsupported markers, or runtime contracts.
- Keep `test_baseline.new.log` treated as a rejected full-suite candidate, not
  an accepted baseline.
- Leave `review/global_address_helper_cleanup_review.md` untouched.

## Proof

No build/test command run; this was a mapping-only Step 1 packet. Do not write
`test_after.log` for this slice.

Exact proof command for code-changing packets:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_(riscv_object_emission|object_model_records|cli_riscv64_.*obj|obj_runtime_rv64_|rv64_roundtrip_contract)'
```
