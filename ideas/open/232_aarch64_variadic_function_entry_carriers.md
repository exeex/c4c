# AArch64 Variadic Function Entry Carriers

Status: Open
Created: 2026-05-14

Parent Context: ideas/open/229_aarch64_codegen_markdown_shards_to_cpp.md

## Problem

Current prepared call plans preserve only the accepted variadic call-boundary
minimum: `PreparedCallWrapperKind::DirectExternVariadic` and
`PreparedCallPlan::variadic_fpr_arg_register_count`. The archived
`variadic.md` and `prologue.md` surfaces also describe full AAPCS64 variadic
function-entry behavior: GP/FP register-save areas, `va_list` layout,
negative register-save offsets, named argument counts, stack overflow-area
policy, `va_start`, `va_arg`, and `va_copy`.

Those full function-entry and `va_list` mechanisms are not represented as
prepared carriers today. Implementing them directly in AArch64 codegen would
recreate local ABI state and frame layout authority in the wrong layer.

## Scope

- Define prepared/shared carriers for AArch64 variadic function-entry metadata
  before machine-node lowering consumes it.
- Carry named GP/FP argument counts, register-save-area slots, stack
  overflow-area base, `va_list` field layout, and helper-call resource needs as
  structured facts.
- Keep call-boundary variadic metadata separate from full variadic function
  entry and `va_arg` lowering.

## Non-Goals

- Do not implement `va_start`, `va_arg`, or `va_copy` by scraping prologue text
  or markdown-era scratch-register conventions.
- Do not create register-save areas or spill slots inside target codegen.
- Do not treat the existing variadic call-boundary count as proof that full
  AAPCS64 `va_list` behavior is implemented.

## Proof Direction

- Prepared dumps expose explicit variadic function-entry facts for a variadic
  callee.
- A later AArch64 machine-node slice can consume those facts without local
  recomputation.
- Variadic call-boundary tests remain separate from `va_start` / `va_arg`
  function-entry tests.
