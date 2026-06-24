# Current Packet

Status: Active
Source Idea Path: ideas/open/332_aarch64_minimal_relocatable_elf_object_emission.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Implement AArch64 Relocatable ELF Object Output For A Smoke Subset

## Just Finished

- Completed Step 4 of `plan.md`: proved the AArch64 object-emission writer
  serializes the supported machine-record external-call subset as a structural
  ELF64 relocatable object.
- Added in-process byte-level assertions for the AArch64 ELF header
  (`EM_AARCH64` 183, flags 0), `.text`, `.rela.text`, `.symtab`, `.strtab`,
  `.shstrtab`, the serialized BL/RET bytes, the defined caller symbol, the
  undefined external function symbol, and `R_AARCH64_CALL26` relocation
  encoding.
- Extended the existing ADRP/ADD fixture through
  `write_aarch64_relocatable_elf_object` and asserted the serialized
  `R_AARCH64_ADR_PREL_PG_HI21` / `R_AARCH64_ADD_ABS_LO12_NC` relocation pair
  against one undefined object symbol.

## Suggested Next

- Implement Step 5 by running the focused object-emission and nearby AArch64
  asm-route validation, then record the handoff limitations for the later
  CLI/default-route integration child without exposing `--codegen obj`.

## Watchouts

- Keep compiler object output independent from printed `.s` text.
- Keep RV64 relocation concepts out of AArch64 APIs.
- Do not expose `--codegen obj`, change defaults, or weaken existing asm-route
  coverage in this child.
- `Aarch64ObjectSymbolKind` currently distinguishes only function and object
  targets for first-slice undefined-symbol construction.
- Structured object emission currently supports only selected return records
  and direct-call records with non-empty `direct_callee_label`.
- Step 4 uses deterministic byte-level ELF assertions instead of external
  readelf/objdump/link tools, so it remains toolchain-independent.
- `EM_AARCH64` is machine `183` with ELF flags `0` for this relocatable writer
  path.
- Known unsupported object features after the first slice: data/global section
  lowering, `.bss`, external CLI `--codegen obj`, c-testsuite/default-route
  scans, real runtime smoke, conditional/test branches, local jumps,
  load/store low-12 relocations, GOT/TLS/dynamic/shared-library relocations,
  variadic and indirect calls, stack/frame-heavy functions, and GNU textual
  assembler compatibility.

## Proof

- Delegated proof passed:
  `set -o pipefail; (cmake --build --preset default && ctest --test-dir build -R '^(backend_aarch64_object_emission|backend_object_model_records|backend_aarch64_target_record_core_contract|backend_aarch64_return_lowering)$' --output-on-failure) > test_after.log 2>&1`
- Test subset: `backend_aarch64_object_emission`,
  `backend_object_model_records`,
  `backend_aarch64_target_record_core_contract`, and
  `backend_aarch64_return_lowering` passed 4/4.
- Proof log path: `test_after.log`.
