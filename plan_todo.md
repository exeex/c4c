# LIR De-Stringify Legacy-Safe Refactor Todo

Status: Active
Source Idea: ideas/open/38_lir_de_stringify_legacy_safe_refactor_plan.md
Source Plan: plan.md

## Active Item

- Step 1: Audit the current string-heavy LIR surfaces and categorize each field
  as typed operand, typed type, enum-like opcode/predicate, or truly textual
  payload.

## Completed Items

- Switched the active plan from
  `ideas/open/35_backend_ready_ir_contract_plan.md` to
  `ideas/open/38_lir_de_stringify_legacy_safe_refactor_plan.md`.
- Parked `35` after confirming that backend-ready IR work should not proceed on
  top of the current string-heavy LIR contract.

## Notes

- The most obvious high-friction string-heavy families currently include:
  `LirLoadOp`, `LirStoreOp`, `LirBinOp`, `LirCmpOp`, `LirCallOp`,
  `LirMemcpyOp`, `LirVaStartOp`, `LirVaEndOp`, `LirVaCopyOp`, `LirAbsOp`,
  `LirIndirectBrOp`, `LirExtractValueOp`, and `LirInsertValueOp`.
- The legacy path impacted by this refactor includes:
  `src/codegen/lir/stmt_emitter.cpp`,
  `src/codegen/lir/hir_to_lir.cpp`,
  `src/codegen/lir/lir_printer.cpp`,
  and any code that still expects pre-rendered LLVM-ish string payloads.
- The backend path impacted by this refactor includes:
  `src/backend/lir_adapter.cpp` and analysis files that currently inspect LIR
  through string payloads.

## Open Questions To Resolve During This Runbook

- Should typed LIR operands distinguish immediates, SSA values, globals, labels,
  and special tokens like `zeroinitializer` as separate variants?
- Should typed LIR types wrap existing LLVM-ish type text temporarily, or map
  directly onto a richer shared type representation?
- Which instruction families should migrate first to maximize consumer cleanup:
  arithmetic/cmp, load/store, or call-adjacent ops?
- Which residual textual payloads are legitimate long-term text, such as inline
  asm source, versus temporary compatibility debt?

## Blockers

- None yet. The immediate task is the migration audit and typed primitive
  design, not backend-ready IR or emitter work.
