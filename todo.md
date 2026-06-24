# Current Packet

Status: Active
Source Idea Path: ideas/open/332_aarch64_minimal_relocatable_elf_object_emission.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect AArch64 Emission And Object API Seams

## Just Finished

- Completed Step 1 of `plan.md`: inspected the AArch64 machine-record,
  printer, module, object-model, and RV64 object-emission seams.
- Chosen seam: add an AArch64-owned object-emission surface under
  `src/backend/mir/aarch64/codegen/object_emission.{hpp,cpp}`. It should consume
  structured `aarch64::module::MachineModule` / `MachineInstruction` records and
  AArch64-owned encoded fragments/fixups, then lower only final target
  relocation numbers into `mir::object::ObjectModule` and the shared ELF writer.
- Do not parse or depend on `MachineInstructionPrinter` output. The printer is
  only the `.s` consumer; the object route should be a sibling consumer over
  structured `InstructionRecord` payloads.
- First minimal smoke subset: one or two `.text` functions with prepared-backed
  `ret` and direct `bl` call records, plus an explicit address-materialization
  fixture for a symbol using ADRP/ADD-style fixups before prepared globals are
  widened.
- Required first relocation kinds:
  `R_AARCH64_CALL26` for direct calls,
  `R_AARCH64_ADR_PREL_PG_HI21` for ADRP page materialization, and
  `R_AARCH64_ADD_ABS_LO12_NC` for ADD low-12 materialization. Keep
  `R_AARCH64_JUMP26`, `R_AARCH64_CONDBR19`, `R_AARCH64_TSTBR14`,
  `R_AARCH64_LDST*_ABS_LO12_NC`, TLS, GOT, and dynamic relocations out of the
  first implementation slice unless the supervisor explicitly widens it.

## Suggested Next

- Implement Step 2 by adding AArch64 encoded-fragment/fixup records and a
  focused `backend_aarch64_object_emission` test that proves direct-call and
  ADRP/ADD fixups can be expressed without printed assembly text.
- Suggested first implementation proof:
  `set -o pipefail; (cmake --build --preset default && ctest --test-dir build -R '^(backend_aarch64_object_emission|backend_object_model_records|backend_aarch64_target_record_core_contract)$' --output-on-failure) > test_after.log 2>&1`

## Watchouts

- Keep compiler object output independent from printed `.s` text.
- Keep RV64 relocation concepts out of AArch64 APIs.
- Do not expose `--codegen obj`, change defaults, or weaken existing asm-route
  coverage in this child.
- Use AArch64-local names such as `Aarch64ObjectFixupKind::Call26`,
  `AdrPrelPgHi21`, and `AddAbsLo12Nc`; raw ELF relocation numbers should stay
  behind an AArch64 mapping helper, not leak into shared object construction.
- `EM_AARCH64` is machine `183` with ELF flags `0` for this relocatable writer
  path.
- Known unsupported object features after the first slice: data/global section
  lowering, `.bss`, external CLI `--codegen obj`, c-testsuite/default-route
  scans, real runtime smoke, conditional/test branches, local jumps,
  load/store low-12 relocations, GOT/TLS/dynamic/shared-library relocations,
  variadic and indirect calls, stack/frame-heavy functions, and GNU textual
  assembler compatibility.

## Proof

- Inspection-only packet; no build required.
- Ran `git diff --check`.
- No `test_after.log` required for this packet.
