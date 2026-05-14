# AArch64 Binary128 Softfloat Lowering

Status: Open
Created: 2026-05-14

Parent Context: ideas/open/229_aarch64_codegen_markdown_shards_to_cpp.md

## Problem

BIR, ABI legalization, and prepared register profiles can represent `F128`
as a type and ABI-facing floating value, but compiled AArch64 codegen does not
yet lower binary128 arithmetic, comparisons, casts, full-width memory
transport, source tracking, temporary slots, or helper-call boundaries into
structured machine nodes. The archived `f128.md` records the valid semantic
shape: full 16-byte values must be preserved and helper calls must not collapse
binary128 values into scalar F64 approximations.

This requires prepared/shared carrier facts plus AArch64 helper-call and
transport nodes; it must not be patched as a scalar float shortcut.

## Scope

- Add prepared carriers for full binary128 source tracking, temporary
  16-byte storage, and helper-call resource/clobber facts where they are
  missing.
- Add AArch64 machine nodes for binary128 full-width load/store/copy,
  constants, sign-bit negation, comparisons, casts, and soft-float helper
  calls.
- Keep F32/F64 scalar FP lowering and i128 pair lowering in separate routes.

## Non-Goals

- Do not assume hardware quad-FP arithmetic on AArch64.
- Do not store truncated F64 approximations into F128 destinations.
- Do not reserve local scratch registers or spill space outside prepared
  authority.
- Do not add fixture-shaped soft-float helper shortcuts.

## Proof Direction

- A binary128 arithmetic operation calls the appropriate helper from
  structured call facts and preserves the full 16-byte result.
- A later F128 comparison or cast reloads the full tracked source, not a
  scalar approximation.
- F128 load/store and copy paths preserve low/high halves and 16-byte storage
  size/alignment.
