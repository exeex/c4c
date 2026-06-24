# Current Packet

Status: Active
Source Idea Path: ideas/open/332_aarch64_minimal_relocatable_elf_object_emission.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Preserve AArch64 Asm Route And Validate Handoff

## Just Finished

- Completed Step 5 of `plan.md`: preserved the existing AArch64 asm route while
  validating the focused object-emission child handoff.
- Ran the delegated build plus object-emission, shared object-model, AArch64
  target-record, return-lowering, machine-printer, and external asm-route CLI
  smoke subset.
- Confirmed the newly added direct AArch64 object-emission coverage remains
  green alongside nearby `.s` route coverage for return-zero, return-add, and
  return-add/sub-chain external toolchain smokes.

## Suggested Next

- Supervisor should ask the plan owner to decide whether this child plan is
  ready to close or whether a lifecycle handoff note should be split for the
  later CLI/default-route integration child.

## Watchouts

- This child did not expose `--codegen obj`, did not switch default codegen
  behavior, and did not run c-testsuite object-route scans.
- Runtime object-route smoke beyond existing asm route validation remains for a
  later CLI/default-route integration child.
- Keep compiler object output independent from printed `.s` text.
- Keep RV64 relocation concepts out of AArch64 APIs.
- `Aarch64ObjectSymbolKind` currently distinguishes only function and object
  targets for first-slice undefined-symbol construction.
- Structured object emission currently supports only selected return records
  and direct-call records with non-empty `direct_callee_label`.
- Step 4 used deterministic byte-level ELF assertions instead of external
  readelf/objdump/link tools, so the object writer proof remains
  toolchain-independent.
- `EM_AARCH64` is machine `183` with ELF flags `0` for this relocatable writer
  path.
- Known unsupported object features after this child: data/global section
  lowering, `.bss`, broader branches, local jumps, load/store low-12
  relocations, GOT/TLS/dynamic/shared-library relocations, variadic and
  indirect calls, stack/frame-heavy functions, GNU textual assembler
  compatibility, external CLI object-route exposure, c-testsuite/default-route
  scans, and runtime object-route smoke beyond existing asm validation.

## Proof

- Delegated proof passed:
  `set -o pipefail; (cmake --build --preset default && ctest --test-dir build -R '^(backend_aarch64_object_emission|backend_object_model_records|backend_aarch64_target_record_core_contract|backend_aarch64_return_lowering|backend_aarch64_machine_printer|backend_cli_aarch64_asm_external_return_zero_smoke|backend_cli_aarch64_asm_external_return_add_smoke|backend_cli_aarch64_asm_external_return_add_sub_chain_smoke)$' --output-on-failure) > test_after.log 2>&1`
- Test subset: `backend_aarch64_object_emission`,
  `backend_object_model_records`,
  `backend_aarch64_target_record_core_contract`,
  `backend_aarch64_return_lowering`, `backend_aarch64_machine_printer`,
  `backend_cli_aarch64_asm_external_return_zero_smoke`,
  `backend_cli_aarch64_asm_external_return_add_smoke`, and
  `backend_cli_aarch64_asm_external_return_add_sub_chain_smoke` passed 8/8.
- Proof log path: `test_after.log`.
