# AArch64 Binary128 Softfloat Lowering

Status: Closed
Created: 2026-05-14
Closed: 2026-05-15

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

## Lifecycle Checkpoint: 2026-05-15

The first active runbook completed its Step 6 representative AArch64 backend
route proof. The proven route covers prepared full-width F128 transport,
soft-float helper boundaries, selected machine records, printer output, and a
representative backend path through load, helper call, store-back, and return.
The supervisor committed that route proof after the backend subset passed
139/139 tests.

At that checkpoint, the source idea remained open because full-width F128
constant payload authority was tracked separately in
`ideas/open/241_f128_full_width_constant_carriers.md`. Remaining unmodeled F128
helper or sign-bit-like operations continued to fail closed until a future
runbook gave them complete prepared helper, carrier, and printer authority.

## Completion Notes

The follow-on runbook consumed the closed full-width constant-carrier
dependency from `ideas/closed/241_f128_full_width_constant_carriers.md` and
completed the remaining parent-route proof. Semantic F128 constants now reach
the existing binary128 backend consumer through structured prepared carrier
facts, while missing, partial, or scalar-only payloads remain fail-closed.

The representative AArch64 backend route proof covers full-width F128 input,
soft-float helper boundary behavior through `__addtf3`, Q-register/Vreg ABI
marshaling, 16-byte memory store-back, and selected return records. Prior route
proof covered prepared full-width transport, helper boundaries, selected
machine records, printer output, comparisons or casts, sign-bit handling, and
load/store preservation.

Atomic, intrinsic, inline-assembly, scalar FP, i128, hardware quad-FP, and
final AArch64 constant assembly-printing behavior remain outside this source
idea's accepted scope.

Close-time regression guard used the executor's matching backend before/after
logs:

`set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } 2>&1 | tee test_after.log`

Result: 139/139 backend tests passed before and after; the non-decreasing
guard mode passed for this lifecycle-only close.
