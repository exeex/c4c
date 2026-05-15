# AArch64 Global Address Materialization

Status: Open
Created: 2026-05-14

Parent Context: ideas/open/229_aarch64_codegen_markdown_shards_to_cpp.md

## Problem

Current BIR and prepared-addressing layers can describe memory accesses backed
by global symbols and string constants, and AArch64 instruction records can
carry symbol-backed memory operands. The archived `globals.md` surface is a
different mechanism: materializing addresses for globals, labels, and
thread-local globals using AArch64 relocation forms such as `ADRP`/low-12
`ADD`, GOT loads, and `tpidr_el0` TLS-relative addressing.

Compiled AArch64 codegen does not yet own selected machine nodes or printer
support for global-address materialization. Label address and TLS address
materialization also need explicit semantic carriers; treating global memory
load/store support as sufficient would hide a real address-producing feature
gap.

## Scope

- Add structured carriers and AArch64 machine nodes for materializing global,
  label, and TLS addresses.
- Preserve GOT/direct/TLS address-kind decisions as target relocation policy,
  not name-string matching.
- Reuse prepared value homes, allocation results, and reserved scratch policy
  for the address result.
- Add printer output only after the address kind and relocation operands are
  explicit structured facts.

## Non-Goals

- Do not rebuild the archived `globals.cpp` scratch-register convention around
  implicit `x0`.
- Do not infer GOT or TLS policy from rendered symbol names.
- Do not conflate global memory load/store lowering with address
  materialization.

## Proof Direction

- Direct global and label address materialization emit both page and low-12
  steps from structured facts.
- GOT-required globals emit a GOT load from explicit relocation policy.
- TLS globals read the thread pointer and apply thread-pointer-relative
  relocation facts before storing the address result.

## Lifecycle Notes

- 2026-05-15: Direct page+low12 global/string-constant materialization, label
  address selection, and explicit GOT policy through selected AArch64
  `GotPageLow12` records have landed. The former GOT-policy prerequisite is
  closed at `ideas/closed/247_explicit_got_materialization_policy.md`.
  Remaining work in this idea is TLS materialization facts, terminal printing
  for label/GOT/TLS where records are complete, and final semantic coverage
  validation.
