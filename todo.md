# Current Packet

Status: Active
Source Idea Path: ideas/open/332_aarch64_minimal_relocatable_elf_object_emission.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Integrate AArch64 Machine Emission With The Object Model

## Just Finished

- Completed Step 3 of `plan.md`: added an AArch64 machine-record object
  emission entry point that consumes structured `MachineFunction` inputs and
  lowers the supported subset through the Step 2 fragment/fixup helpers.
- Supported selected `ReturnInstructionRecord` as `ret` and selected
  `CallInstructionRecord` with `MachineOpcode::DirectCall` as `bl` plus
  `R_AARCH64_CALL26`, without reading printed assembly text.
- Added explicit object-emission diagnostics for missing function inputs,
  missing function names, unsupported calls, and unsupported machine records
  instead of silently falling back to `.s`.
- Extended `backend_aarch64_object_emission` coverage for return-only,
  same-module direct call, external direct call, unsupported-record rejection,
  and preserved ADRP/ADD address-materialization fixture support.

## Suggested Next

- Implement Step 4 by adding the narrow public object emission handoff needed
  by the next route, while keeping CLI/runtime/default-route exposure and
  broader prepared globals/data/BSS support out of scope until explicitly
  planned.

## Watchouts

- Keep compiler object output independent from printed `.s` text.
- Keep RV64 relocation concepts out of AArch64 APIs.
- Do not expose `--codegen obj`, change defaults, or weaken existing asm-route
  coverage in this child.
- `Aarch64ObjectSymbolKind` currently distinguishes only function and object
  targets for first-slice undefined-symbol construction.
- Structured object emission currently supports only selected return records
  and direct-call records with non-empty `direct_callee_label`.
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
  `backend_object_model_records`, and
  `backend_aarch64_target_record_core_contract`, and
  `backend_aarch64_return_lowering` passed 4/4.
- Proof log path: `test_after.log`.
