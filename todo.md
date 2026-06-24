Status: Active
Source Idea Path: ideas/open/331_rv64_minimal_relocatable_elf_object_emission.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add RV64 Encoded Fragment And Fixup Records

# Current Packet

## Just Finished

Step 2 implementation completed for RV64-owned encoded fragment/fixup records.

Added `src/backend/mir/riscv/codegen/object_emission.hpp` and `.cpp` with:

- `RiscvEncodedFragment` carrying instruction bytes plus typed
  `RiscvObjectFixup` records.
- target-local `RiscvObjectFixupKind::CallPlt` lowering to ELF relocation
  number `R_RISCV_CALL_PLT` (`19`).
- RV64 relocatable ELF config (`EM_RISCV` machine `243`, flags `0x5`).
- helpers for the first minimal text subset: `auipc ra, 0` + `jalr ra, 0(ra)`
  direct-call pair, return-zero bytes, text-section object-module construction,
  and shared-writer RV64 ELF serialization.

Added `tests/backend/mir/backend_riscv_object_emission_test.cpp` and CMake
registration. The focused test proves `.text`, a defined global function
symbol, an undefined global callee symbol, same-module direct-call symbol
resolution, `R_RISCV_CALL_PLT` relocation lowering, and RV64 ELF header/config
serialization through the shared writer. No printed assembly text is used by the
direct object helper or the test.

## Suggested Next

Implement Step 3 by integrating the RV64 object helper with the prepared
module/function surfaces for the same minimal `.text` + direct-call subset.
Keep unsupported `.data`, `.bss`, global-address materialization, branch/jump
relocations, and full `pcrel_hi` / `pcrel_lo` pairing explicit rather than
falling back to the asm route.

## Watchouts

- Step 2 keeps call-pair relocation atomic with `R_RISCV_CALL_PLT`; it does not
  implement synthetic AUIPC-site labels or split `pcrel_hi` / `pcrel_lo`
  relocation records yet.
- The helper is target-owned and direct-object-only; it does not parse or reuse
  printed `.s` text.
- Same-module call targets are represented by defining the target function in
  the same object module; this packet's focused ELF assertion separately uses
  the undefined callee contract requested by the supervisor.
- Still unsupported after this slice: `.data`, `.bss`, `.rodata`, string
  constants, local branch/jump relocations, global load/store address
  materialization, full `pcrel_hi`/`pcrel_lo` pairing, GOT/TLS relocations,
  linker relaxation hints, compressed instruction emission, CLI `--codegen obj`,
  and runtime/link smoke.

## Proof

Delegated proof passed:

```sh
set -o pipefail; (cmake --build --preset default && ctest --test-dir build -R '^(backend_riscv_object_emission|backend_object_model_records)$' --output-on-failure) > test_after.log 2>&1
```

Result: 2/2 tests passed (`backend_object_model_records`,
`backend_riscv_object_emission`). Proof log: `test_after.log`.
