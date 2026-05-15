# AArch64 Variadic Function Entry Carriers

Status: Closed
Created: 2026-05-14
Closed: 2026-05-15

Parent Context: ideas/closed/229_aarch64_codegen_markdown_shards_to_cpp.md
Follow-up: ideas/open/243_aarch64_variadic_machine_node_consumption.md

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

## Completion Notes

The active route added prepared variadic-entry carriers, populated AAPCS64
callee entry facts in prepared state, exposed those facts through prepared
dumps and focused tests, and added fail-closed AArch64 helper guards so target
lowering cannot silently reconstruct missing ABI facts.

Closure proof used the backend close gate:

`(cmake --build build -j2 && ctest --test-dir build -j --output-on-failure -R '^backend_')`

Result: 139/139 backend tests passed. The c4c regression guard passed in
non-decreasing mode against the matching backend logs.

Final `va_start`, scalar `va_arg`, aggregate `va_arg`, and `va_copy`
machine-node consumption remains intentionally deferred. That downstream scope
is split into `ideas/open/243_aarch64_variadic_machine_node_consumption.md`
because it requires complete prepared/frame storage and scratch-resource facts
before AArch64 selected machine nodes can materialize helper side effects.
