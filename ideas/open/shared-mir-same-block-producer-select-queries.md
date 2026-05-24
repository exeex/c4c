# Shared MIR Same-Block Producer And Select-Chain Queries

## Intent

Move same-block producer discovery and select-chain dependency analysis out of
AArch64 codegen into a shared MIR query surface that AArch64, x86, and RISC-V
lowering can reuse before final target instruction selection.

## Why This Exists

`dispatch_producers.cpp` currently owns broad local def-use facts used by
AArch64 call arguments, branch fusion, value publication, and store handling.
Those facts are not AArch64 instruction spelling. x86 and RISC-V will need the
same block-local producer lookup, constant discovery, select-chain dependency,
and support-instruction filtering decisions before they choose target mnemonics
or registers.

This idea is a focused first migration slice from the second-wave AArch64
codegen audit.

## In Scope

- Introduce a shared MIR query API for same-block producer lookup.
- Represent select-chain dependency facts in target-neutral records.
- Move only the generic query and dependency traversal logic needed by current
  AArch64 users.
- Keep AArch64 call, branch-fusion, publication, and store users consuming the
  new shared query without changing emitted machine instructions.
- Add a small x86 or RISC-V reuse fixture, adapter, or consumer-facing unit
  test that proves the query shape is not AArch64-only.

## Out Of Scope

- AArch64 instruction spelling, ABI lane policy, stack-pointer sequences,
  memory operand spelling, and final machine instruction construction.
- Target condition-code mapping, compare/branch mnemonic choice, or immediate
  encodability policy.
- Moving publication planning, store-source planning, or call-boundary ordering.
- Renaming helper declarations without moving ownership of the underlying query
  responsibility.

## Acceptance Criteria

- Same-block producer and select-chain queries live in a shared MIR-owned or
  target-neutral helper location, not in AArch64-only codegen ownership.
- Existing AArch64 behavior for current call, branch-fusion, publication, and
  store users remains equivalent.
- At least one x86 or RISC-V path can include or exercise the shared query
  surface without depending on AArch64 namespaces or emitted instruction forms.
- The AArch64 codegen layer still owns final target emission and target policy.

## Proof Expectations

- Build proof for affected C++ targets.
- AArch64 parity tests or focused fixtures for same-block producer and
  select-chain cases already covered by the current users.
- One x86 or RISC-V compile/unit fixture proving the shared query can be
  consumed outside AArch64 codegen.

## Reviewer Reject Signals

- The change adds named-testcase shortcuts or recognizes only one hard-coded
  producer/select shape instead of exposing a general same-block query.
- AArch64 branch, call, publication, or store tests are weakened, marked
  unsupported, or rewritten to avoid existing behavior.
- The new shared API encodes AArch64 register names, condition mnemonics,
  memory operands, ADRP/ADD forms, or final machine instruction construction.
- The patch is mostly helper renames or declaration moves while the old
  AArch64-owned query logic remains the real owner.
- The purported x86/RISC-V reuse proof is only a comment or unused include with
  no compile or fixture coverage.
