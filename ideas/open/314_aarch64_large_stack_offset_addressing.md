# AArch64 Large Stack Offset Addressing

Status: Open
Created: 2026-05-19
Split From: ideas/closed/312_aarch64_lir_to_bir_local_memory_prepared_handoff.md

## Goal

Repair AArch64 stack-slot memory addressing and stack-backed scalar
publication when generated frame offsets exceed the immediate range accepted by
the selected load/store instruction form.

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

Idea 313 later closed the `f128_transport` printer owner and parked a sibling
large-offset publication residual here. The `00204.c` representative now fails
after f128 transport has been repaired with:

```text
cannot print AArch64 machine node family=scalar opcode=add:
scalar ALU stack publication offset is not printable
```

That residual is also a large stack-offset addressing problem: a selected
scalar ALU result is being published to stack-backed storage through a direct
stack offset that the AArch64 printer cannot legally spell.

## In Scope

- Identify the AArch64 memory instruction forms and offset ranges used for
  stack-slot loads/stores and stack-backed scalar result publication.
- Repair lowering or printing so large stack offsets are addressed through a
  legal base-plus-offset sequence instead of an out-of-range immediate form.
- Preserve correct frame-slot identity, width, alignment, and signedness
  semantics while materializing large offsets.
- Add focused backend machine-printer, dispatch, or target-instruction
  coverage for large stack-offset loads/stores and scalar stack publications
  before relying on external representatives.
- Use `c_testsuite_aarch64_backend_src_00216_c` as the representative external
  case for large stack-slot load/store offset validation.
- Use `c_testsuite_aarch64_backend_src_00204_c` as the representative external
  case for scalar ALU stack-publication offset validation.

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
- Filename-only, offset-literal-only, function-name-only,
  diagnostic-string-only, scalar-opcode-only, or c-testsuite-number-specific
  fixes.

## Acceptance Criteria

- Focused backend coverage proves AArch64 large stack-offset loads/stores and
  stack-backed scalar result publications use legal instruction sequences for
  offsets outside the selected immediate range.
- `c_testsuite_aarch64_backend_src_00216_c` advances past the current
  `ldr x13, [sp, #1644]` assembly-validation failure, or any later first-bad
  fact is classified as outside this owner.
- `c_testsuite_aarch64_backend_src_00204_c` advances past the current scalar
  ALU stack-publication offset printer failure, or any later first-bad fact is
  classified as outside this owner.
- Existing semantic/prepared-handoff tests from idea 312 remain green.
- Existing f128 transport focused tests from idea 313 remain green.
- Fresh build and focused CTest proof are recorded before closure.

## Reviewer Reject Signals

Reject the route if it:

- special-cases `00216.c`, `00204.c`, `test_zero_init`, `stdarg`, offset
  `#1644`, one stack slot, one scalar opcode, or one assembler/printer
  diagnostic instead of repairing legal large stack-offset addressing;
- changes expectations, unsupported classifications, allowlists, timeout
  policy, runner behavior, CTest registration, proof-log contents, or test
  contracts to improve counts;
- claims progress while equivalent out-of-range stack loads/stores or
  stack-backed scalar publications can still reach emitted or printable AArch64
  forms through the same selected instruction family;
- folds `f128_transport`, semantic admission, runtime mismatch, timeout,
  direct-call, vararg, or unrelated frame-layout work into this owner without a
  separate lifecycle split;
- broadens into global frame-layout rewrites without focused evidence and
  matching backend coverage for the large-offset addressing contract.
