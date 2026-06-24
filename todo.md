Status: Active
Source Idea Path: ideas/open/331_rv64_minimal_relocatable_elf_object_emission.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Implement RV64 Relocatable ELF Object Output For A Smoke Subset

# Current Packet

## Just Finished

Step 4 continuation completed for RV64 PC-relative HI/LO relocation pairing in
target-owned object emission records and focused ELF serialization tests.

Added RV64 typed fixups for `PcrelHi20` and `PcrelLo12I`, plus fragment-local
synthetic label records. Added `make_rv64_pcrel_address_fragment()` so the high
relocation targets the final symbol and the low relocation targets the synthetic
AUIPC-site label. Extended `backend_riscv_object_emission` with structural
object-model and serialized ELF checks for relocation offsets, numeric
relocation types `23`/`24`, addends, symbol names, and the AUIPC-site label
rule.

## Suggested Next

Implement the next packet for Step 5: validate the RV64 child scope against the
prepared-backed direct-call and structural PCREL object tests, then hand off
remaining CLI/default-route/runtime expansion to later child ideas.

## Watchouts

- The PCREL proof is structural object proof only: it constructs a minimal
  target-owned fixture and serializes it through the shared writer, but it does
  not connect global address materialization from prepared BIR yet.
- The prepared-backed object helper intentionally rejects unsupported global
  storage, string constants, arguments/results, locals, atomics, multi-block
  functions, branches/jumps, indirect calls, variadic extern calls, stack
  arguments, call results, and memory returns instead of falling back to asm.
- Still unsupported after this slice: `.data`, `.bss`, `.rodata`, global
  load/store address materialization from prepared BIR, `PcrelLo12S`, GOT/TLS
  relocations, linker relaxation hints, compressed instruction emission, CLI
  `--codegen obj`, c-testsuite integration, and full runtime smoke.
- Runtime smoke is not practical in this slice because there is still no CLI
  object-output route, GCC driver, startup harness, or supported broader RV64
  object subset; the practical proof is structural ELF validation plus
  `riscv64-linux-gnu-ld -r`.

## Proof

Delegated proof passed:

```sh
set -o pipefail; (cmake --build --preset default && ctest --test-dir build -R '^(backend_riscv_object_emission|backend_object_model_records)$' --output-on-failure) > test_after.log 2>&1
```

Result: 2/2 tests passed (`backend_object_model_records`,
`backend_riscv_object_emission`). The RV64 object test now covers direct-call
ELF output plus structural `R_RISCV_PCREL_HI20`/`R_RISCV_PCREL_LO12_I`
serialization with the low relocation referencing the synthetic AUIPC-site label
symbol. Proof log: `test_after.log`.
