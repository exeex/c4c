Status: Active
Source Idea Path: ideas/open/331_rv64_minimal_relocatable_elf_object_emission.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Preserve RV64 Asm Route And Validate Handoff

# Current Packet

## Just Finished

Step 5 completed for RV64 minimal relocatable ELF handoff validation.

Recorded supervisor validation for the RV64 child scope. The accepted proof
covers the prepared-backed direct-call object path, structural RV64 PC-relative
HI/LO relocation pairing, the shared object writer coverage, and nearby existing
RV64 asm-route coverage so the assembler route remains independently green.

## Suggested Next

Ask the plan owner to decide whether child 331 can close and hand off the
remaining CLI/test/default-route/runtime work to the later native object child
ideas.

## Watchouts

- The RV64 object route is intentionally limited to prepared-backed `.text`,
  direct void calls, function symbols, direct-call relocations, and structural
  PCREL HI/LO fixture coverage.
- The PCREL proof remains structural target-owned object proof; prepared-BIR
  global address materialization is not wired into the object route yet.
- The prepared-backed object helper intentionally rejects unsupported global
  storage, string constants, arguments/results, locals, atomics, multi-block
  functions, branches/jumps, indirect calls, variadic extern calls, stack
  arguments, call results, and memory returns instead of silently falling back
  to asm.
- Later child ideas still own CLI `--codegen obj` exposure, c-testsuite object
  integration, default-route readiness scans, runtime smoke, global storage,
  `.data`, `.bss`, `.rodata`, `PcrelLo12S`, GOT/TLS relocations, linker
  relaxation hints, compressed instruction emission, and broader RV64 object
  subset expansion.

## Proof

Supervisor validation was already run for this Step 5 handoff:

```sh
set -o pipefail; (cmake --build --preset default && ctest --test-dir build -R '^(backend_riscv_object_emission|backend_object_model_records|backend_riscv_prepared_edge_publication|backend_codegen_route_riscv64_external_no_storage_main_emits_return_path|backend_codegen_route_riscv64_external_user_unresolved_diagnostic)$' --output-on-failure) > test_after.log 2>&1
```

Result: 5/5 tests passed (`backend_riscv_object_emission`,
`backend_object_model_records`, `backend_riscv_prepared_edge_publication`,
`backend_codegen_route_riscv64_external_no_storage_main_emits_return_path`,
`backend_codegen_route_riscv64_external_user_unresolved_diagnostic`). The proof
keeps the object model/writer and RV64 object tests green while preserving
nearby RV64 asm-route coverage. Proof log: `test_after.log`.
