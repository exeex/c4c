# AArch64 Large Stack Offset Addressing

Status: Open
Created: 2026-05-19
Split From: ideas/closed/312_aarch64_lir_to_bir_local_memory_prepared_handoff.md

## Goal

Repair AArch64 stack-slot memory addressing when generated frame offsets exceed
the immediate range accepted by the selected load/store instruction form.

## Why This Exists

Idea 312 closed after `00216.c` advanced past the semantic `lir_to_bir`
local-memory prepared-handoff diagnostics. The current residual is now backend
assembly validation, not semantic admission:

```asm
ldr x13, [sp, #1644]
```

The assembler rejects this at
`build/c_testsuite_aarch64_backend/src/00216.c.s:1514:19` with:

```text
index must be an integer in range [-256, 255]
```

This owner is about legal AArch64 stack-offset address formation or
materialization for large frame offsets after assembly has been emitted.

## In Scope

- Identify the AArch64 memory instruction forms and offset ranges used for
  stack-slot loads/stores.
- Repair lowering or printing so large stack offsets are addressed through a
  legal base-plus-offset sequence instead of an out-of-range immediate form.
- Preserve correct frame-slot identity, width, alignment, and signedness
  semantics while materializing large offsets.
- Add focused backend machine-printer, dispatch, or target-instruction
  coverage for large stack-offset loads/stores before relying on the external
  representative.
- Use `c_testsuite_aarch64_backend_src_00216_c` as the representative external
  case for the large-offset residual.

## Out Of Scope

- Reopening idea 312's semantic local-memory prepared-handoff owner.
- Repairing `f128_transport` printer residuals, runtime mismatches, linker
  behavior, timeout behavior, direct-call shuffle, direct vararg, or
  address-of-local residuals.
- Reworking frame layout globally unless focused evidence shows legal large
  stack-offset addressing cannot be repaired at the lowering/printing boundary.
- Expectation rewrites, unsupported-classification changes, allowlist changes,
  runner edits, timeout-policy changes, proof-log edits, or CTest
  registration changes.
- Filename-only, offset-literal-only, function-name-only, diagnostic-string-only,
  or c-testsuite-number-specific fixes.

## Acceptance Criteria

- Focused backend coverage proves AArch64 large stack-offset loads/stores use
  legal instruction sequences for offsets outside the selected immediate
  range.
- `c_testsuite_aarch64_backend_src_00216_c` advances past the current
  `ldr x13, [sp, #1644]` assembly-validation failure, or any later first-bad
  fact is classified as outside this owner.
- Existing semantic/prepared-handoff tests from idea 312 remain green.
- Fresh build and focused CTest proof are recorded before closure.

## Reviewer Reject Signals

Reject the route if it:

- special-cases `00216.c`, `test_zero_init`, offset `#1644`, one stack slot, or
  one assembler diagnostic instead of repairing legal large stack-offset
  addressing;
- changes expectations, unsupported classifications, allowlists, timeout
  policy, runner behavior, CTest registration, proof-log contents, or test
  contracts to improve counts;
- claims progress while equivalent out-of-range stack loads/stores can still
  reach emitted AArch64 assembly through the same selected instruction family;
- folds `f128_transport`, semantic admission, runtime mismatch, timeout,
  direct-call, vararg, or unrelated frame-layout work into this owner without a
  separate lifecycle split;
- broadens into global frame-layout rewrites without focused evidence and
  matching backend coverage for the large-offset addressing contract.
