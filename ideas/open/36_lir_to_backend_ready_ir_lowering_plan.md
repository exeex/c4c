# LIR To Backend Ready-IR Lowering Plan

## Status

Open as of 2026-03-29.

## Relationship To Roadmap

Umbrella source: `ideas/open/__backend_port_plan.md`

Depends on:

- `ideas/open/38_lir_de_stringify_legacy_safe_refactor_plan.md`
- `ideas/open/35_backend_ready_ir_contract_plan.md`

Should precede:

- `ideas/open/37_backend_emitter_backend_ir_migration_plan.md`
- `ideas/open/34_backend_x86_c_testsuite_backend_convergence_plan.md`

## Goal

Replace the current minimal adapter with a real lowering pipeline from
`c4c::codegen::lir::LirModule` into the new backend-ready IR contract.

## Why This Slice Exists

The missing layer is not "a few more special cases in `lir_adapter.cpp`".
What is missing is a general normalization pass that turns frontend-shaped LIR
into backend-owned IR with predictable control-flow and memory semantics.

Without that layer, testsuite progress mostly measures how many shapes were
hardcoded into the adapter, not whether the backend can consume real compiler
IR.

## Primary Scope

- `src/codegen/lir/ir.hpp`
- `src/backend/lir_adapter.hpp`
- `src/backend/lir_adapter.cpp`
- `src/backend/backend.cpp`
- new lowering passes under `src/backend/`

## Proposed Files

Add:

- `src/backend/lir_to_backend_ir.hpp`
- `src/backend/lir_to_backend_ir.cpp`
- `src/backend/lir_cfg_normalize.hpp`
- `src/backend/lir_cfg_normalize.cpp`
- `src/backend/lir_mem2reg.hpp`
- `src/backend/lir_mem2reg.cpp`
- `src/backend/lir_phi_eliminate.hpp`
- `src/backend/lir_phi_eliminate.cpp`

Shrink and eventually delete:

- `src/backend/lir_adapter.hpp`
- `src/backend/lir_adapter.cpp`

## Proposed API Surface

Add the main lowering entrypoint:

- `BackendModule lower_to_backend_ir(const c4c::codegen::lir::LirModule& module);`

Add pass-level helpers usable from focused tests:

- `void normalize_lir_cfg(c4c::codegen::lir::LirFunction& function);`
- `void promote_lir_allocas(c4c::codegen::lir::LirFunction& function);`
- `void eliminate_lir_phi(c4c::codegen::lir::LirFunction& function);`

Expected lowering pipeline inside `lower_to_backend_ir(...)`:

1. normalize block order / terminator invariants
2. promote eligible entry allocas and trivial load/store traffic
3. either preserve phi in structured form or eliminate phi before backend IR
   materialization
4. lower globals / string pool / extern decls into backend IR entities
5. materialize structured calls, branches, compares, and returns
6. validate the result before emitter entry

## Required Behavior

The new lowering layer must handle at least:

- multi-block functions
- `LirBr`, `LirCondBr`, and `LirRet`
- `LirCmpOp`
- `LirLoadOp` / `LirStoreOp` / `LirAllocaOp` for promotable scalar cases
- direct calls represented as structured callee + arguments
- globals, string pool entries, and extern decls at module scope

It must stop relying on:

- hardcoded function-name checks like `signature.name == "main"`
- block-count special cases for "single-block only"
- string-spliced argument normalization
- constant interpretation loops that simulate execution just to produce a
  simpler shape

## Reference Files

Use these reference files for decomposition and invariants:

- `ref/claudes-c-compiler/src/ir/README.md`
- `ref/claudes-c-compiler/src/ir/analysis.rs`
- `ref/claudes-c-compiler/src/ir/mem2reg/promote.rs`
- `ref/claudes-c-compiler/src/ir/mem2reg/phi_eliminate.rs`
- `ref/claudes-c-compiler/src/backend/README.md`
- `ref/claudes-c-compiler/src/backend/generation.rs`

## Non-Goals

- no target-specific register allocation in this idea
- no x86 peephole or AArch64 emission expansion here
- no broad testsuite bucketing until the lowering boundary exists

## Validation

- add focused tests for lowering from representative LIR fixtures into
  backend-ready IR dumps
- verify that multi-block, branchy, memory-using inputs can reach backend IR
  without fallback to LLVM text
- ensure globals / string constants / extern decls no longer fail at module
  ingest time

## Suggested Execution Order

1. land CFG normalization and invariants first
2. land promotable alloca/load/store lowering
3. land phi policy and implementation
4. land module-scope lowering for globals / strings / externs
5. switch backend dispatch to use `lower_to_backend_ir(...)`
6. retire the legacy minimal adapter or leave it as a test-only shim

## Success Condition

This idea is complete when backend ingress is driven by a general
`LIR -> backend-ready IR` pipeline rather than `lir_adapter.cpp` shape
recognition.
