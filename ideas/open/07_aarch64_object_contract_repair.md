# AArch64 Object Contract Repair

Status: Open
Last Updated: 2026-04-02

## Goal

Repair the remaining AArch64 backend object-contract failures:

- `backend_contract_aarch64_return_add_object`
- `backend_contract_aarch64_global_load_object`

## Background

The backend regression-recovery plan closed on 2026-04-02 after recovering the broader aarch64/x86/runtime regression cluster. The only backend failures left in `ctest --test-dir build -R backend --output-on-failure` are these two AArch64 contract cases.

Both currently fail before objdump validation because the backend asm output file is empty:

- `return_add_aarch64.s` is missing `.text`
- `global_load_aarch64.s` is missing `.data`

That leaves a narrower object-emission gap in the AArch64 contract path rather than the prior mixed regression cluster.

## Scope

- Stay inside the AArch64 backend contract/object-emission path.
- Preserve the recovered x86/runtime backend boundary from the prior plan.
- Validate with the contract tests and the full backend sweep before closing.

## Workstreams

### 1) Reproduce and classify the empty-output failure

- Capture the exact compiler/toolchain invocation used by `run_backend_contract_case.cmake`.
- Determine whether the blank `.s` files come from frontend failure, backend emission failure, or the external toolchain handoff.

### 2) Repair return/global-load object emission

- Fix `backend_contract_aarch64_return_add_object`.
- Fix `backend_contract_aarch64_global_load_object`.
- Add or adjust the narrowest tests needed to lock the repaired emission behavior.

### 3) Validate and hand off

- Run:
  - `ctest --test-dir build -R 'backend_contract_aarch64_(return_add_object|global_load_object)' --output-on-failure`
  - `ctest --test-dir build -R backend --output-on-failure`
- Document any remaining contract gaps if additional AArch64 binary-utils work is still deferred.

## Exit Criteria

- [ ] `backend_contract_aarch64_return_add_object` passes
- [ ] `backend_contract_aarch64_global_load_object` passes
- [ ] `ctest --test-dir build -R backend --output-on-failure` no longer fails in these two contract cases
