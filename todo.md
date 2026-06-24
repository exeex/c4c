# Current Packet

Status: Active
Source Idea Path: ideas/open/333_codegen_obj_cli_and_test_integration.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect CLI And Backend Output-Form Seams

## Just Finished

- Step 1 inspection recorded the `--codegen obj` output-form seam for child
  333.

Chosen integration seam: keep text and object output as separate backend/codegen
result paths. The current CLI accepts `--codegen llvm|asm|compare`; `asm` routes
through `CodegenPath::Lir`, receives text from `emit_module_native(...)`, and
writes it with `std::ofstream << text`. The object route should not widen that
text path. Add a backend-level object-byte result API separate from the existing
text emitters, with `std::vector<std::uint8_t> bytes` plus a stable diagnostic
or error status, then route by target from LIR through prepared BIR into
target-native object emitters.

First supported target subset for integration:

- RV64: wire from LIR to BIR/prepared BIR and then
  `write_rv64_prepared_relocatable_elf_object(const PreparedBirModule&)`.
- AArch64: wire through the accepted AArch64 object model path and
  `write_aarch64_relocatable_elf_object`; add a prepared-module convenience
  bridge if needed via existing AArch64 machine/module construction before CLI
  exposure.
- x86, unknown targets, and unsupported object features must fail clearly at the
  backend object entrypoint and hand that diagnostic to the CLI. They must never
  fall back to asm or `BackendAssembleResult`.

Likely Step 2 owned files:

- `src/backend/backend.hpp`
- `src/backend/backend.cpp`
- `src/backend/mir/riscv/codegen/object_emission.*` only if facade wiring needs
  a stable include or result adapter
- `src/backend/mir/aarch64/codegen/object_emission.*` only if a prepared-module
  bridge is required
- focused backend tests for the facade object-byte entrypoints

Diagnostic ownership: backend object entrypoints should own target/support
diagnostics such as unsupported target, missing target object emitter, or
unsupported prepared object feature. CLI should own option spelling, output-file
opening/writing failures, and formatting the backend diagnostic without silently
selecting asm.

## Suggested Next

- Step 2 should add the backend object-byte entrypoints and tests without
  exposing `--codegen obj` yet. Keep `src/codegen/llvm/llvm_codegen.hpp/.cpp`
  text-only until Step 3, then add a driver/codegen facade for object bytes and
  make the CLI write bytes with `ostream::write`, not `operator<<`.

Recommended Step 2 proof command:

```sh
set -o pipefail; (cmake --build --preset default && ctest --test-dir build -R '^(backend_object_model_records|backend_riscv_object_emission|backend_aarch64_object_emission|backend_aarch64_target_record_core_contract|backend_aarch64_return_lowering)$' --output-on-failure) > test_after.log 2>&1
```

CTest/helper direction for later steps: add object-route CMake helper(s) beside
the existing asm route helpers, not in place of them. Keep route labels precise:
`object`, `asm`, `dual-route`, and target labels such as `riscv64` and `aarch64`
so supervisors can select object-only, asm-only, or coexistence proof.

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

## Proof

- Inspection-only todo update; no build required by delegation.
- Local validation: `git diff --check`.
