# Current Packet

Status: Active
Source Idea Path: ideas/open/333_codegen_obj_cli_and_test_integration.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Backend Object Output Entrypoints

## Just Finished

- Step 2 added backend-level native object output entrypoints without exposing
  `--codegen obj` through the CLI.
- `src/backend/backend.hpp` now exposes `BackendObjectResult` with object bytes
  plus a stable diagnostic string, alongside separate BIR/LIR/public
  `emit_*_object(...)` entrypoints.
- `src/backend/backend.cpp` routes RV64 object output through semantic
  BIR/prepared BIR into
  `write_rv64_prepared_relocatable_elf_object(...)`.
- `src/backend/backend.cpp` routes AArch64 object output through semantic
  BIR/prepared BIR, `compile_prepared_module(...)`, target machine object
  module construction, and `write_aarch64_relocatable_elf_object(...)`.
- Unsupported targets and unsupported prepared object shapes return diagnostics
  and no bytes; they do not fall back to asm and do not reuse
  `BackendAssembleResult`.
- `backend_object_model_records` now covers the backend facade success path for
  RV64/AArch64 ELF bytes, unsupported x86 diagnostics, unsupported RV64 globals,
  and preservation of the existing asm text API.

## Suggested Next

- Step 3 should add the driver/codegen facade for object bytes and wire
  `--codegen obj` through the CLI. Keep `--codegen asm` on the existing text
  route, and write object bytes with `ostream::write`, not `operator<<`.
- Add CLI tests for accepted RV64/AArch64 object output, unsupported target
  diagnostics, and asm/object coexistence. Keep object-route CMake helpers
  beside existing asm helpers, with `object`, `asm`, `dual-route`, `riscv64`,
  and `aarch64` labels where useful.

## Watchouts

- Do not implement `--codegen obj` by printing `.s` and invoking or parsing an
  assembler.
- Do not remove, rename away, or weaken existing `--codegen asm` tests.
- Do not make c-testsuite default to object output in this child.
- Split unrelated backend semantic gaps into focused follow-up ideas instead
  of absorbing them into CLI wiring.
- `src/codegen/llvm/llvm_codegen.hpp/.cpp` currently expose text-only
  `CodegenPath::{Llvm,Lir,Compare}` and `emit_module_native(...) -> std::string`;
  Step 2 should avoid forcing object bytes through that string API.
- `BackendAssembleResult` is bootstrap/staging compatibility and should not
  become the direct object-byte CLI surface.
- Unsupported object combinations should be stable negative tests, not
  fallback-to-asm behavior.
- AArch64 object facade currently clears prepared regalloc before compiling,
  matching the existing AArch64 assembly route behavior in `backend.cpp`.
- RV64 globals/data remain unsupported by the prepared object writer in this
  child and are intentionally diagnostic-only until later object-route work.

## Proof

- Passed:

```sh
set -o pipefail; (cmake --build --preset default && ctest --test-dir build -R '^(backend_object_model_records|backend_riscv_object_emission|backend_aarch64_object_emission|backend_aarch64_target_record_core_contract|backend_aarch64_return_lowering)$' --output-on-failure) > test_after.log 2>&1
```

- Result: 5/5 tests passed.
- Proof log: `test_after.log`.
