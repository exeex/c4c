# Current Packet

Status: Active
Source Idea Path: ideas/open/333_codegen_obj_cli_and_test_integration.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Wire --codegen obj Through The CLI

## Just Finished

- Step 3 wired `--codegen obj` through the codegen facade and CLI without
  forcing object bytes through the existing text-returning
  `emit_module_native(...)` path.
- `src/codegen/llvm/llvm_codegen.hpp/.cpp` now expose
  `NativeObjectResult` and `emit_module_native_object(...)`, lower HIR to LIR,
  and call the backend object-byte facade when backend support is enabled.
- `src/apps/c4cll.cpp` now accepts `--codegen obj`, requires `-o <path>` for
  object output, surfaces backend diagnostics as `error: --codegen obj failed`,
  and writes ELF bytes with binary `ostream::write`.
- The CLI keeps `--codegen asm` on the existing text route; the selected proof
  keeps the AArch64 asm smoke test beside the new object tests.
- Added focused CMake object-route helpers and CLI tests for RV64/AArch64 ELF
  object bytes, unsupported x86 object diagnostics, unsupported RV64 global
  object diagnostics, and the `--codegen obj` output-path policy.

## Suggested Next

- Step 4 should extend the object-route smoke harness toward the first
  compile/link/runtime proof, reusing the new CLI object helpers where possible
  and keeping asm route coverage separate.

## Watchouts

- Do not implement `--codegen obj` by printing `.s` and invoking or parsing an
  assembler.
- Do not remove, rename away, or weaken existing `--codegen asm` tests.
- Do not make c-testsuite default to object output in this child.
- Split unrelated backend semantic gaps into focused follow-up ideas instead
  of absorbing them into CLI wiring.
- Object stdout remains intentionally unsupported in this slice; `--codegen obj`
  requires `-o <path>` so the CLI never mixes binary output with diagnostics.
- `--backend-bir-stage semantic` remains asm-only.
- Unsupported object combinations should continue to fail through backend/CLI
  diagnostic handoff, never by falling back to asm.
- RV64 globals/data remain unsupported by the prepared object writer in this
  child and are intentionally diagnostic-only until later object-route work.

## Proof

- Passed:

```sh
set -o pipefail; (cmake --build --preset default && ctest --test-dir build -R '^(backend_object_model_records|backend_cli_.*obj|backend_cli_aarch64_asm_external_return_zero_smoke|backend_codegen_route_riscv64_external_no_storage_main_emits_return_path)$' --output-on-failure) > test_after.log 2>&1
```

- Result: 8/8 tests passed.
- Proof log: `test_after.log`.
