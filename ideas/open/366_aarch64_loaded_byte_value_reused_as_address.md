# AArch64 Loaded Byte Value Reused As Address

Status: Open
Created: 2026-05-21
Split From: ideas/closed/365_aarch64_string_literal_pointer_value_publication.md

## Goal

Repair the AArch64/prepared scalar-memory path that reuses a loaded byte value
as a pointer address after the string literal pointer value has already been
published correctly.

## Why This Exists

Idea 365 fixed the earlier `00173` failure where `char *a = "hello"` stored a
stack-spill address into the local pointer slot instead of the `.str0` address.
After commit 79ea7bceb, generated AArch64 publishes the real literal address
into `%lv.a`.

The remaining representative failure is later and separate. The generated path
loads the current pointer and performs an initial byte load, then treats the
loaded byte/scalar value as if it were an address for a subsequent byte load.
The recorded first-bad-fact shape is:

- `ldr x13, [sp, #16]`
- `ldrb w9, [x13]`
- `mov x9, x13; ldrb w13, [x9]`
- after `sxtb w13, w13`, another `mov x9, x13; ldrb w13, [x9]`

This is not string/global address pointer-value publication. It is a
scalar-memory/address-carrier distinction problem in prepared or generated
AArch64 emission.

## In Scope

- Localize why a loaded byte/scalar value remains or becomes address-capable
  during prepared AArch64 emission.
- Repair the general scalar-memory or address-carrier rule so loaded byte
  values are not reused as pointer addresses.
- Prove the `00173` residual advances beyond this byte-value-as-address first
  bad fact or passes.
- Preserve the completed idea 365 contract that string/global addresses are
  published as pointer values before local pointer storage.
- Preserve idea 356's dynamic pointer-derived byte-load behavior.

## Out Of Scope

- Reopening string/global pointer-value publication from idea 365 unless fresh
  evidence shows the exact old `sp+offset` publication failure returned.
- Reopening semantic dynamic pointer-derived byte-load preservation from idea
  356 except to keep that behavior stable.
- Direct external-call string/global argument lowering, recursive formal-home
  publication, frontend admission, ABI composite/byval/HFA/f128, variadic
  floating, dynamic stack, runner behavior, timeout policy, CTest
  registration, proof-log handling, expectation changes, or unsupported
  classification changes.

## Acceptance Criteria

- The first boundary where the loaded byte/scalar value becomes an address
  operand is localized with prepared BIR, register-allocation, or generated
  AArch64 evidence.
- Focused backend coverage proves loaded byte/scalar values are kept distinct
  from pointer/address carriers.
- `c_testsuite_aarch64_backend_src_00173_c` advances beyond the recorded
  byte-value-as-address failure or passes.
- Existing string/global pointer publication coverage remains green.
- No expectation, unsupported-classification, runner, timeout, CTest
  registration, proof-log, or filename-specific changes are used as progress.

## Reviewer Reject Signals

Reject the route if it:

- special-cases `00173`, one literal, one stack slot, one register, one emitted
  instruction sequence, or one loop body instead of repairing the scalar
  value/address-carrier rule;
- weakens expectations, unsupported classifications, runner behavior, timeout
  policy, proof-log policy, CTest registration, or external test contracts;
- claims progress through helper renames, emitted-text rewrites, or
  classification notes while a loaded byte value can still be used as a memory
  address;
- reopens the already completed idea 365 pointer-publication path without
  fresh evidence that the exact old `sp+offset` pointer publication failure
  returned;
- reintroduces fixed global-byte loads for dynamic `*p` / `*src` consumers
  that idea 356 repaired;
- broadens into unrelated call lowering, ABI composite, variadic/floating,
  frontend, dynamic stack, or runner work without fresh first-bad-fact
  evidence and a lifecycle split.

