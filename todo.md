Status: Active
Source Idea Path: ideas/open/543_rv64_object_data_symbol_fixup_module_cleanup.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Review Text Fixup And Symbol Publication Boundary

# Current Packet

## Just Finished

Step 3 reviewed local label publication, undefined symbol handling, text
relocation mapping, and symbol binding in `build_rv64_text_object_module`.

Moved helpers:
- `publish_rv64_text_fragment_label` now owns local fragment-label validation,
  label binding, local no-type symbol definition, and insertion into the
  explicit `symbols_by_name` publication map.
- `attach_rv64_text_fixup` now owns text-fixup validation, relocation type
  mapping through `rv64_elf_relocation_type`, defined-or-undefined target
  symbol lookup/publication through the explicit `symbols_by_name` map, and
  text relocation attachment at `fragment_section_offset + fixup.offset_bytes`.

Explicit dependencies kept visible: `ObjectModule`, `.text` section id,
`symbols_by_name`, fragment section offset, fragment byte size, and the concrete
`RiscvObjectLabel` or `RiscvObjectFixup`. The main text-module owner still owns
function byte ordering, fragment append order, function symbol sizing/binding,
and the final `ObjectModule` result.

Parked helpers: data-object pointer relocation handling remains in
`append_rv64_prepared_data_objects`; final module/ELF assembly, public object
entrypoints, section writer ownership, and function-byte ordering were not
moved in this packet. Text fixup attachment remains separate from data pointer
relocations because they still use different producer contracts and relocation
inputs.

## Suggested Next

Execute Step 4 by reviewing final module assembly only. Extract code only if
the API-compatible wrapper keeps section layout, relocation ownership, public
entrypoints, and ELF config explicit.

## Watchouts

- Preserve object bytes, relocations, symbol bindings, section layout, ELF
  flags, public entrypoints, and zero-fill behavior.
- Keep text fixup attachment separate from data-object pointer relocation
  handling; Step 3 intentionally split only the text-side helper path.
- Step 4 should not move final module assembly if doing so would hide `.text`,
  `.rodata`, `.data`, `.bss`, relocation, section ordering, or ELF flag
  ownership behind a broad context.
- Do not touch tests, expectations, unsupported markers, or runtime contracts.
- Keep `test_baseline.new.log` treated as a rejected full-suite candidate, not
  an accepted baseline.
- Leave `review/global_address_helper_cleanup_review.md` untouched.

## Proof

Code-changing Step 3 proof passed and was written to `test_after.log`: 34/34
tests passed.

Exact command run:

```sh
bash -o pipefail -c "cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_(riscv_object_emission|object_model_records|cli_riscv64_.*obj|obj_runtime_rv64_|rv64_roundtrip_contract)'" > test_after.log 2>&1
```
