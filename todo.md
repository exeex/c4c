Status: Active
Source Idea Path: ideas/open/331_rv64_minimal_relocatable_elf_object_emission.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Implement RV64 Relocatable ELF Object Output For A Smoke Subset

# Current Packet

## Just Finished

Step 4 implementation completed for RV64 relocatable ELF object output on the
prepared-backed minimal `.text` plus direct-call subset.

Added `write_rv64_prepared_relocatable_elf_object()` as the RV64-owned wrapper
that builds the prepared text object module and serializes it through the shared
relocatable ELF writer. Extended `backend_riscv_object_emission` so the
prepared-backed path is serialized to ELF bytes, the test parses the RV64 ELF
header, `.text`, `.rela.text`, `.symtab`, `R_RISCV_CALL_PLT`, and defined
`caller`/`callee` function symbols, then writes the bytes to a temp object file.

The temp object is accepted by `readelf`, `llvm-objdump -r`, and
`riscv64-linux-gnu-ld -r` without routing through printed assembly.

## Suggested Next

Implement the next packet for Step 5: validate the RV64 child scope and hand off
the remaining unsupported object-route work to the later child ideas without
expanding this minimal prepared-backed subset.

## Watchouts

- Step 4 keeps call-pair relocation atomic with `R_RISCV_CALL_PLT`; it does not
  implement synthetic AUIPC-site labels or split `pcrel_hi` / `pcrel_lo`
  relocation records yet.
- The prepared-backed object helper intentionally rejects unsupported global
  storage, string constants, arguments/results, locals, atomics, multi-block
  functions, branches/jumps, indirect calls, variadic extern calls, stack
  arguments, call results, and memory returns instead of falling back to asm.
- Still unsupported after this slice: `.data`, `.bss`, `.rodata`, global
  load/store address materialization, full `pcrel_hi`/`pcrel_lo` pairing,
  GOT/TLS relocations, linker relaxation hints, compressed instruction
  emission, CLI `--codegen obj`, c-testsuite integration, and full runtime
  smoke.
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
`backend_riscv_object_emission`). The RV64 object test also invoked `readelf`,
`llvm-objdump -r`, and `riscv64-linux-gnu-ld -r` on the prepared-backed temp
object file. Proof log: `test_after.log`.
