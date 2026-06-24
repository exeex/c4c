# Target Object Emitter Scalar Scan Expansion

## Goal

Expand target-owned native object emission enough for the first object-route
scan scalar cases to pass for RV64 and AArch64, so the broader object-route
scan/default-readiness child can continue with green selectable scan cases.

## Why This Exists

The object-route scan in `ideas/open/334_object_route_scan_and_default_readiness.md`
blocked before link/runtime. The harness could select object cases, but target
object emission rejected simple scalar arithmetic, local, and call-shaped
modules:

- RV64 `return_add.c`, `two_arg_helper.c`, `return_add_sub_chain.c`, and
  `local_temp.c` failed with `RISC-V backend object route unsupported prepared
  module shape`.
- AArch64 `return_add.c` and `return_add_sub_chain.c` failed with
  `unsupported AArch64 machine instruction for object emission`.

These are prerequisite target object-emitter capabilities, not scan/default
policy decisions.

## In Scope

- Inspect the exact RV64 prepared-module shapes and AArch64 machine
  instructions produced by the failing scalar candidates.
- Add target-owned object emission support for the minimal scalar/no-global
  subset needed by those candidates.
- Keep all object output routed through native object emitters and shared ELF
  writing; do not use textual assembly or external assemblers as the compiler
  object path.
- Add focused target/backend tests for the newly supported object records or
  emitted ELF bytes where they provide useful ownership proof.
- Re-run the same scan candidate names that blocked idea 334 and prove they
  pass without expected-failure labels or asm fallback.

## Out Of Scope

- Broad RV64 globals/data object support.
- x86 object output.
- c-testsuite object defaults.
- object stdout policy.
- object `--backend-bir-stage semantic` mode.
- AArch64 runtime execution unless a reliable runner already exists and the
  executor packet explicitly owns it.
- Textual assembler support.

## Acceptance And Proof Expectations

- RV64 object route supports the first scalar/no-global runtime candidates:
  `return_add.c`, `two_arg_helper.c`, `return_add_sub_chain.c`, and
  `local_temp.c`.
- AArch64 object route supports byte emission for scalar/no-global candidates:
  `return_add.c` and `return_add_sub_chain.c`.
- The object scan candidate tests can be added back as green tests selected by
  `backend_obj_runtime_.*` and/or `backend_cli_.*obj`.
- Existing child-333 object smoke tests and nearby asm-route tests still pass.
- Any remaining unsupported target feature is recorded as a focused follow-up
  rather than hidden behind expected-failure scan coverage.

## Dependency Notes

- Split from `ideas/open/334_object_route_scan_and_default_readiness.md` after
  Step 2 proved scan expansion is blocked by target object-emitter support.
- Depends on closed direct object support from children 331, 332, and 333.
- Feeds idea 334 by unblocking the first scalar object-route scan subset.

## Reviewer Reject Signals

- The compiler object path prints `.s`, invokes an assembler, or parses
  textual assembly to make these cases pass.
- The route adds expected-failure labels, unsupported classifications, or
  weaker tests for the exact scalar candidates while claiming capability
  progress.
- The implementation special-cases filenames, function names, or one named
  testcase shape instead of supporting the target object records/instructions
  that the cases produce.
- RV64 scalar support is claimed while the prepared module still fails with
  `unsupported prepared module shape` for nearby scalar/no-global cases.
- AArch64 scalar support is claimed while the object writer still rejects the
  produced machine instruction family with the same unsupported-instruction
  diagnostic.
- Existing asm-route or child-333 object tests are removed, renamed away, or
  weakened to make the new proof look green.
- Broad globals/data, c-testsuite defaults, x86 object output, object stdout,
  or semantic-BIR object mode is absorbed into this focused child.

## Closure Notes

Closed after the focused target-emitter scalar expansion unblocked the first
object-route scan candidate set.

Completed support:

- RV64 object runtime scalar coverage for `return_add.c`, `two_arg_helper.c`,
  `return_add_sub_chain.c`, and `local_temp.c`.
- AArch64 object-byte scalar coverage for `return_add.c` and
  `return_add_sub_chain.c`.
- Restored object-route selectors:
  `backend_obj_runtime_rv64_return_zero`,
  `backend_obj_runtime_rv64_return_add`,
  `backend_obj_runtime_rv64_two_arg_helper`,
  `backend_obj_runtime_rv64_return_add_sub_chain`,
  `backend_obj_runtime_rv64_local_temp`,
  `backend_cli_aarch64_return_zero_writes_elf_obj`,
  `backend_cli_aarch64_return_add_writes_elf_obj`, and
  `backend_cli_aarch64_return_add_sub_chain_writes_elf_obj`.

Close proof:

```bash
set -o pipefail; (cmake --build --preset default && ctest --test-dir build -R '^(backend_object_model_records|backend_riscv_object_emission|backend_aarch64_object_emission|backend_cli_.*obj|backend_obj_runtime_.*|backend_rv64_runtime_(return_zero|return_add|two_arg_helper|two_arg_local_arg|two_arg_both_local_arg|two_arg_second_local_arg|local_arg_call|return_add_sub_chain|local_temp)|backend_cli_aarch64_asm_external_return_(zero|add|add_sub_chain)_smoke|backend_codegen_route_riscv64_external_no_storage_main_emits_return_path)$' --output-on-failure) > test_after.log 2>&1
```

Result: 28/28 tests passed. Regression guard passed with matching
`test_before.log` and `test_after.log` using non-decreasing pass-count mode.

Remaining broader object scan/default-readiness work belongs to
`ideas/open/334_object_route_scan_and_default_readiness.md`.
