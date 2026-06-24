Status: Active
Source Idea Path: ideas/open/331_rv64_minimal_relocatable_elf_object_emission.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Integrate RV64 Machine Emission With The Object Model

# Current Packet

## Just Finished

Step 3 implementation completed for integrating RV64 prepared module/function
surfaces with the shared object model for the minimal `.text` plus direct-call
subset.

Added `build_rv64_prepared_text_object_module()` in the RV64-owned object
emission helper. The path consumes `PreparedBirModule` control-flow entries and
`PreparedCallPlan` data, converts supported direct void calls into encoded RV64
call fragments, appends return-zero fragments, and publishes `.text`, function
symbols, and `R_RISCV_CALL_PLT` relocations through shared object helpers.

Extended `backend_riscv_object_emission` with a synthetic prepared module whose
BIR `CallInst` has no callee text; the callee exists only in the prepared call
plan. The test proves the direct object path uses prepared data, not printed
assembly text, and rejects unsupported globals without falling back to asm.

## Suggested Next

Implement Step 4 by supplying RV64 ELF object-output proof for the same
prepared-backed minimal subset, including structural relocation assertions and
link/runtime smoke only if the local toolchain can prove it without routing
through printed assembly.

## Watchouts

- Step 3 keeps call-pair relocation atomic with `R_RISCV_CALL_PLT`; it does not
  implement synthetic AUIPC-site labels or split `pcrel_hi` / `pcrel_lo`
  relocation records yet.
- The prepared-backed object helper intentionally rejects unsupported global
  storage, string constants, arguments/results, locals, atomics, multi-block
  functions, branches/jumps, indirect calls, variadic extern calls, stack
  arguments, call results, and memory returns instead of falling back to asm.
- Still unsupported after this slice: `.data`, `.bss`, `.rodata`, global
  load/store address materialization, full `pcrel_hi`/`pcrel_lo` pairing,
  GOT/TLS relocations, linker relaxation hints, compressed instruction
  emission, CLI `--codegen obj`, and runtime/link smoke.

## Proof

Delegated proof passed:

```sh
set -o pipefail; (cmake --build --preset default && ctest --test-dir build -R '^(backend_riscv_object_emission|backend_object_model_records)$' --output-on-failure) > test_after.log 2>&1
```

Result: 2/2 tests passed (`backend_object_model_records`,
`backend_riscv_object_emission`). Proof log: `test_after.log`.
