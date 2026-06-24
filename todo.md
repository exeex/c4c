# Current Packet

Status: Active
Source Idea Path: ideas/open/333_codegen_obj_cli_and_test_integration.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Add Object-Route Smoke Harness Support

## Just Finished

- Step 4 added a focused RV64 object-route runtime smoke harness beside the
  existing asm runtime harness.
- `tests/backend/cmake/run_backend_rv64_object_runtime_case.cmake` emits
  `--codegen obj` directly to `.o`, checks ELF magic and RV64 `e_machine`,
  links the object with clang using the existing riscv64 toolchain convention,
  and runs the linked binary with `qemu-riscv64`.
- `tests/backend/CMakeLists.txt` now has
  `c4c_add_backend_rv64_object_runtime_case(...)` and registers
  `backend_obj_runtime_rv64_return_zero` for `tests/backend/case/return_zero.c`.
- Existing asm-route runtime coverage and CLI object/asm tests were preserved;
  the selected proof runs both `backend_rv64_runtime_return_zero` and the new
  object runtime smoke.

## Suggested Next

- Step 5 should decide whether the active object-route scope is complete or
  whether a narrow follow-up should add one more supported object smoke, such as
  an AArch64 runtime proof only if the local toolchain is equally reliable.

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

## Proof

- Passed:

```sh
set -o pipefail; (cmake --build --preset default && ctest --test-dir build -R '^(backend_object_model_records|backend_cli_.*obj|backend_obj_runtime_.*|backend_rv64_runtime_return_zero|backend_cli_aarch64_asm_external_return_zero_smoke|backend_codegen_route_riscv64_external_no_storage_main_emits_return_path)$' --output-on-failure) > test_after.log 2>&1
```

- Result: 10/10 tests passed.
- Proof log: `test_after.log`.
