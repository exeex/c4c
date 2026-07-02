Status: Active
Source Idea Path: ideas/open/543_rv64_object_data_symbol_fixup_module_cleanup.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Review Data Object And Relocation Boundary

# Current Packet

## Just Finished

Step 2 reviewed `append_rv64_prepared_data_objects` and made no implementation
changes.

No narrow data-object helper boundary was extracted. The useful unit still has
to keep the following dependencies visible together:
- selected object-data contract facts and unsupported-but-coherent fallback
  status for each global
- zero-fill classification tied to the original global initializer shape,
  object-data identity, object size, alignment, and BSS section selection
- section selection for `.bss`, `.rodata`, and `.data`, including section
  flags and alignment before byte append or reservation
- pointer-initializer relocation creation through
  `rv64_selected_symbol_pointer_initializer_label`,
  `rv64_find_or_declare_relocation_symbol`, `rv64_prepared_link_symbol_kind`,
  and `attach_relocation` with `kRiscvReloc64`
- final data symbol definition through
  `rv64_prepared_object_data_symbol_binding`, selected section id, offset, and
  object size

Extracting a per-global helper would need to carry `prepared`, `global`,
selected object-data status, section ownership, symbol lookup/definition, and
relocation attachment as one broad context. Extracting only section selection,
zero-fill reservation, or pointer relocation attachment would hide exactly the
layout and relocation contracts this step is meant to keep explicit. Text fixup
attachment remains separate from data pointer relocation handling.

Moved helpers: none.

Parked helpers: data-object helper extraction stays parked until a stable
prepared object-data relocation contract can expose relocation facts without
falling back to BIR global initializer inspection inside the object emitter.

## Suggested Next

Execute Step 3 by reviewing text fixup and symbol publication only. Keep text
fixup attachment separate from data-object pointer relocation handling unless a
stable shared contract already exists.

## Watchouts

- Preserve object bytes, relocations, symbol bindings, section layout, ELF
  flags, public entrypoints, and zero-fill behavior.
- Keep text fixup attachment separate from data-object pointer relocation
  handling unless a stable shared contract already exists.
- Step 2 intentionally left `append_rv64_prepared_data_objects` central because
  the remaining state is narrower and clearer in place than behind a broad
  context object.
- Do not touch tests, expectations, unsupported markers, or runtime contracts.
- Keep `test_baseline.new.log` treated as a rejected full-suite candidate, not
  an accepted baseline.
- Leave `review/global_address_helper_cleanup_review.md` untouched.

## Proof

No build/test command run; this was a no-code Step 2 packet. No
`test_after.log` was written.

Exact proof command for code-changing packets:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_(riscv_object_emission|object_model_records|cli_riscv64_.*obj|obj_runtime_rv64_|rv64_roundtrip_contract)'
```
