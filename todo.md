# Current Packet

Status: Active
Source Idea Path: ideas/open/332_aarch64_minimal_relocatable_elf_object_emission.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add AArch64 Encoded Fragment And Fixup Records

## Just Finished

- Completed Step 2 of `plan.md`: added AArch64-owned encoded fragment and typed
  fixup records under
  `src/backend/mir/aarch64/codegen/object_emission.{hpp,cpp}`.
- Added target-local helpers for `ret`, direct `bl` calls, and ADRP/ADD
  address materialization placeholders without depending on printed assembly
  text.
- Added AArch64-local relocation mapping for `Aarch64ObjectFixupKind::Call26`,
  `AdrPrelPgHi21`, and `AddAbsLo12Nc` to `R_AARCH64_CALL26`,
  `R_AARCH64_ADR_PREL_PG_HI21`, and `R_AARCH64_ADD_ABS_LO12_NC`.
- Added focused `backend_aarch64_object_emission` coverage proving `.text`
  bytes, function symbols, undefined and same-module direct-call symbol
  handling, and ADRP/ADD relocation pairs against an object symbol.

## Suggested Next

- Implement Step 3 by wiring a narrow AArch64 machine/module object builder into
  the new target-local records for the supported minimal subset, keeping
  unsupported prepared forms explicit instead of falling back to printed `.s`.

## Watchouts

- Keep compiler object output independent from printed `.s` text.
- Keep RV64 relocation concepts out of AArch64 APIs.
- Do not expose `--codegen obj`, change defaults, or weaken existing asm-route
  coverage in this child.
- `Aarch64ObjectSymbolKind` currently distinguishes only function and object
  targets for first-slice undefined-symbol construction.
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
  `set -o pipefail; (cmake --build --preset default && ctest --test-dir build -R '^(backend_aarch64_object_emission|backend_object_model_records|backend_aarch64_target_record_core_contract)$' --output-on-failure) > test_after.log 2>&1`
- Test subset: `backend_aarch64_object_emission`,
  `backend_object_model_records`, and
  `backend_aarch64_target_record_core_contract` passed 3/3.
- Proof log path: `test_after.log`.
