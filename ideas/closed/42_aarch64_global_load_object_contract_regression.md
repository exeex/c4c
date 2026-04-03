# AArch64 Global Load Object Contract Regression

Status: Complete
Last Updated: 2026-04-03

## Goal

Repair the reintroduced `backend_contract_aarch64_global_load_object` failure
without regressing the recently recovered AArch64 fallback-global-load runtime
cluster.

## Background

After the AArch64 fallback normalization fix for reused global bases, the
contract test `backend_contract_aarch64_global_load_object` now fails because
the emitted asm no longer contains:

- `ldr w0, [x8, :lo12:g_counter]`

Instead it now emits:

- `adrp x8, g_counter`
- `add x8, x8, :lo12:g_counter`
- `ldr w0, [x8]`

That is semantically valid for runtime execution, but it changes the object
contract surface and the relocation kind expected by the contract test.

## Scope

- `src/apps/c4cll.cpp`
- `tests/c/internal/InternalTests.cmake`
- `tests/c/internal/cmake/run_backend_contract_case.cmake` only if validation
  wording needs adjustment

## Non-Goals

- no rollback of the recovered AArch64 c-testsuite runtime fix
- no broad redesign of AArch64 asm normalization
- no unrelated backend or BIR work

## Suggested Approach

1. Reproduce `backend_contract_aarch64_global_load_object` and inspect the
   current normalized fallback asm.
2. Update AArch64 fallback normalization so single-use global loads keep the
   original `ldr ..., :lo12:symbol` contract, while cases that reuse the base
   register for later offset loads still materialize the full address via
   `add`.
3. Validate both:
   - `backend_contract_aarch64_global_load_object`
   - the previously recovered AArch64 fallback-global-load runtime subset

## Success Criteria

- `backend_contract_aarch64_global_load_object` passes again
- the AArch64 c-testsuite global-load regressions remain fixed

## Completion Notes

- 2026-04-03: Reproduced the failure and confirmed it was not a new lowering
  regression. It was a fallout from the earlier AArch64 fallback asm
  normalization change in
  [src/apps/c4cll.cpp](/workspaces/c4c/src/apps/c4cll.cpp).
- 2026-04-03: The normalization step had been upgraded to always rewrite
  `adrp :got:sym` / `ldr xN, [xN, :got_lo12:sym]` / `ldr wM, [xN]` into
  `adrp + add + ldr [xN]`, which preserved runtime correctness for reused-base
  loads but broke the object contract that expects the single-load
  `ldr w0, [x8, :lo12:g_counter]` surface.
- 2026-04-03: Refined normalization to distinguish:
  - single-use global loads: keep `ldr ..., :lo12:symbol`
  - reused-base patterns with later `[xN, #offset]` accesses: materialize the
    full symbol address with `add`
- 2026-04-03: Validation passed:
  - `ctest --test-dir build -R '^backend_contract_aarch64_global_load_object$' --output-on-failure`
  - `ctest --test-dir build -R '^c_testsuite_aarch64_backend_src_(00040|00047|00048|00049|00050|00090|00091|00092|00146|00147|00148|00149|00150)_c$' --output-on-failure`
