# Codegen Obj CLI And Test Integration

## Goal

Expose `--codegen obj` through the compiler CLI and test infrastructure once
the shared object model and at least one target object emitter are ready, while
preserving existing `--codegen asm` coverage as a compatibility route.

## Why This Exists

Native object emission needs a user-visible output form and test route. The
CLI and tests must distinguish target, backend route, and output form so object
proof can grow beside the existing assembly route without weakening current
coverage or prematurely changing defaults.

## In Scope

- Add or wire `--codegen obj` output selection for supported target/backend
  combinations.
- Ensure object output writes `.o` bytes directly from backend object emission
  and reports unsupported target/output combinations clearly.
- Add focused CLI tests for output-form selection, diagnostics, output
  filenames, and coexistence with `--codegen asm`.
- Add backend test harness support for object-route compile/link/runtime smoke
  tests beside asm-route tests.
- Keep labels, CTest filters, and route names clear enough for supervisors to
  select object-only, asm-only, and dual-route subsets.

## Out Of Scope

- Designing the shared object model.
- Implementing RV64 or AArch64 instruction encoding or relocations.
- Making c-testsuite default to object output.
- Removing or replacing existing asm-route tests.
- Building textual assembler support.

## Acceptance And Proof Expectations

- CLI tests prove `--codegen obj` accepts supported target/output combinations
  and rejects unsupported ones with stable diagnostics.
- Backend tests can run object-route smoke cases independently from asm-route
  cases.
- Dual-route proof demonstrates the asm route remains available and tested.
- The object route links compiler-produced `.o` files without relying on
  printed assembly as an intermediate compiler artifact.

## Dependency Notes

- Depends on `ideas/open/330_native_object_model_and_emission_api.md`.
- Should wait for at least one accepted target object-emission child, preferably
  RV64 first, before advertising supported object output.
- AArch64 CLI support may be gated until
  `ideas/open/332_aarch64_minimal_relocatable_elf_object_emission.md` closes.
- Feeds the broader object-route scan/default-readiness child.

## Reviewer Reject Signals

- `--codegen obj` invokes the asm writer and an assembler as the compiler's
  primary object path.
- Existing asm-route tests are removed, renamed away, or weakened rather than
  supplemented with object-route tests.
- Unsupported target/output combinations silently fall back to asm or produce a
  misleading success result.
- Test labels do not let supervisors distinguish asm-route failures from
  object-route failures.
- The slice claims CLI integration progress while only changing expectation
  names or test classification.
- The change starts c-testsuite default switching before target object proof is
  accepted.

## Closure Note

Closed after child 333 completed the scoped CLI and harness integration:
`--codegen obj` writes backend-produced object bytes for the minimal RV64 and
AArch64 supported subsets, focused CLI tests cover accepted and unsupported
object combinations, the RV64 object route has a direct `.o` link/run smoke,
and existing asm-route proof remains selected.

Final focused validation used:

```sh
set -o pipefail; (cmake --build --preset default && ctest --test-dir build -R '^(backend_object_model_records|backend_riscv_object_emission|backend_aarch64_object_emission|backend_cli_.*obj|backend_obj_runtime_.*|backend_rv64_runtime_return_zero|backend_cli_aarch64_asm_external_return_(zero|add|add_sub_chain)_smoke|backend_codegen_route_riscv64_external_no_storage_main_emits_return_path)$' --output-on-failure) > test_after.log 2>&1
```

Result: 14/14 tests passed. Matching `test_before.log` and regenerated
`test_after.log` both passed 14/14; regression guard passed with
`--allow-non-decreasing-passed`.

Remaining broader object-route scan/default-readiness work is intentionally
left to `ideas/open/334_object_route_scan_and_default_readiness.md`, including
broader RV64 globals/data, x86 object output, object stdout policy, c-testsuite
object defaults, and any object `--backend-bir-stage semantic` mode.
