# Current Packet

Status: Active
Source Idea Path: ideas/open/333_codegen_obj_cli_and_test_integration.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Validate Handoff To Object-Route Scan

## Just Finished

- Step 5 ran the final focused validation for child 333 without changing
  implementation, test, or CMake files.
- `--codegen obj` is now exposed for the minimal RV64 and AArch64 object output
  subset, with direct binary writes to `-o <path>` and no object stdout policy.
- Backend object emission supports RV64 and AArch64 through the backend object
  facade. Unsupported x86 targets and unsupported RV64 globals/data remain
  diagnostic-only and do not fall back to asm.
- Focused CLI object coverage now includes RV64/AArch64 ELF object bytes,
  unsupported x86 diagnostics, unsupported RV64 global diagnostics, and missing
  `-o` diagnostics.
- Runtime object coverage now includes
  `backend_obj_runtime_rv64_return_zero`, which links the compiler-produced
  `.o` directly and runs it with `qemu-riscv64`.
- Final validation also kept existing asm-route proof selected: the RV64 asm
  route smoke and AArch64 external return zero/add/add-sub-chain smokes.

## Suggested Next

- Hand off to `ideas/open/334_object_route_scan_and_default_readiness.md` for
  object-route scan/default-readiness work. Start from the selectable object
  groups and unsupported-feature notes below rather than widening child 333.

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
- The new object runtime helper intentionally links the `.o` directly and never
  reads, writes, or assembles `.s`.
- RV64 globals/data remain unsupported by the prepared object writer in this
  child and are intentionally diagnostic-only until later object-route work.
- Useful selectable groups after child 333: `backend_cli_.*obj` for CLI object
  bytes/diagnostics, `backend_obj_runtime_.*` for object runtime smoke,
  `backend_riscv_object_emission` and `backend_aarch64_object_emission` for
  target-owned object model coverage, and labels `object`, `backend_runtime`,
  `rv64`, `qemu`, `case`, and `smoke` on the RV64 object runtime test.
- Remaining unsupported object-route features include broader RV64 globals/data,
  x86 object output, object stdout, defaulting c-testsuite to object output,
  and any `--backend-bir-stage semantic` object mode.

## Proof

- Passed:

```sh
set -o pipefail; (cmake --build --preset default && ctest --test-dir build -R '^(backend_object_model_records|backend_riscv_object_emission|backend_aarch64_object_emission|backend_cli_.*obj|backend_obj_runtime_.*|backend_rv64_runtime_return_zero|backend_cli_aarch64_asm_external_return_(zero|add|add_sub_chain)_smoke|backend_codegen_route_riscv64_external_no_storage_main_emits_return_path)$' --output-on-failure) > test_after.log 2>&1
```

- Result: 14/14 tests passed.
- Proof log: `test_after.log`.
