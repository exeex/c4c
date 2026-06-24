# Current Packet

Status: Active
Source Idea Path: ideas/open/334_object_route_scan_and_default_readiness.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Selectable Object Scan Helpers

## Just Finished

- Step 2 attempted to add selectable object-route scan cases beside existing
  asm-route cases, but the first scalar/no-global expansion is blocked by
  target object-emitter support rather than CMake harness wiring.
- Attempted RV64 object runtime cases for `return_add.c`, `two_arg_helper.c`,
  `return_add_sub_chain.c`, and `local_temp.c` all failed during
  `--codegen obj` emission with `RISC-V backend object route unsupported
  prepared module shape`, before link or qemu runtime.
- Attempted AArch64 object-byte cases for `return_add.c` and
  `return_add_sub_chain.c` failed during `--codegen obj` emission with
  `AArch64 backend object route unsupported machine module shape: unsupported
  AArch64 machine instruction for object emission`.
- The failing CTest additions were removed from `tests/backend/CMakeLists.txt`
  so the tree does not keep known-red scan tests. Existing object selectors
  remain at the child-333 baseline: `backend_cli_.*obj`,
  `backend_obj_runtime_rv64_return_zero`, `backend_riscv_object_emission`,
  `backend_aarch64_object_emission`, and `backend_object_model_records`.

## Suggested Next

- Split focused follow-up work before continuing scan expansion. The smallest
  useful next packet is target-owned object-emitter capability triage for the
  first scalar arithmetic/local cases: RV64 prepared object module support for
  nonzero constants, ALU/local/call-shaped prepared modules, and AArch64 object
  writer support for the machine instructions emitted by `return_add.c`.
- After that target work lands, rerun Step 2 with the same candidate names and
  proof command rather than weakening expectations or converting these cases
  into expected-failure scan coverage.

## Watchouts

- Do not weaken or remove asm-route coverage while adding object-route scan
  selectors.
- Do not mark broad failures unsupported as a substitute for triage.
- Keep c-testsuite default-output decisions out of the first inspection packet.
- Keep the first scan away from known unsupported/default-policy boundaries:
  RV64 globals/data in object output, x86 object output, object stdout,
  c-testsuite object defaults, and object `--backend-bir-stage semantic`.
- The current blocker is not a missing clang/qemu toolchain path; all failures
  occurred before link/runtime.
- Do not add expected-failure scan labels for the attempted scalar cases as a
  substitute for target object-emitter support.

## Proof

- Failed as expected for the attempted expansion:

```sh
set -o pipefail; (cmake --build --preset default && ctest --test-dir build -R '^(backend_object_model_records|backend_riscv_object_emission|backend_aarch64_object_emission|backend_cli_.*obj|backend_obj_runtime_.*|backend_rv64_runtime_(return_zero|return_add|two_arg_helper|two_arg_local_arg|two_arg_both_local_arg|two_arg_second_local_arg|local_arg_call|return_add_sub_chain|local_temp)|backend_cli_aarch64_asm_external_return_(zero|add|add_sub_chain)_smoke|backend_codegen_route_riscv64_external_no_storage_main_emits_return_path)$' --output-on-failure) > test_after.log 2>&1
```

- Result: 22/28 tests passed; 6 attempted new object scan cases failed at
  object emission.
- Failed tests:
  `backend_cli_aarch64_return_add_writes_elf_obj`,
  `backend_cli_aarch64_return_add_sub_chain_writes_elf_obj`,
  `backend_obj_runtime_rv64_return_add`,
  `backend_obj_runtime_rv64_two_arg_helper`,
  `backend_obj_runtime_rv64_return_add_sub_chain`, and
  `backend_obj_runtime_rv64_local_temp`.
- Proof log: `test_after.log`.
- Cleanup check passed after removing the known-red CTest additions:

```sh
git diff --check
```
