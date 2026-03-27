# Backend LIR Adapter Todo

Status: Active
Source Idea: ideas/open/01_backend_lir_adapter_plan.md
Source Plan: plan.md

## Current Active Item

- Step 2: add backend skeleton and entry wiring

## Next Intended Slice

- add `src/backend/` scaffolding with a target enum and backend entry API
- route the existing post-HIR codegen handoff through the new backend entry boundary without changing current LLVM text output
- keep the first adapter input contract limited to `c4c::codegen::lir::LirModule`, `LirFunction`, `LirBlock`, `LirInst`, and `LirTerminator`

## Completed Items

- activated `ideas/open/01_backend_lir_adapter_plan.md` into `plan.md`
- initialized execution-state tracking for the active plan
- Step 1 complete: current compiler handoff is `src/apps/c4cll.cpp -> c4c::codegen::llvm_backend::emit_module_native()` in `src/codegen/llvm/llvm_codegen.cpp`, which immediately calls `c4c::codegen::lir::lower(const c4c::hir::Module&)` and then `lir::print_llvm(...)`
- Step 1 complete: the current LIR-side adapter seam is centered on `src/codegen/lir/hir_to_lir.hpp`, `src/codegen/lir/ir.hpp`, and `src/codegen/lir/stmt_emitter.hpp`; the smallest backend-facing payload for return-only bring-up is `LirModule -> LirFunction -> LirBlock -> {LirInst, LirTerminator}`
- Step 1 complete: the minimal ref backend entry shape to mirror first is `ref/claudes-c-compiler/src/backend/mod.rs` `Target` + top-level dispatch, with future assembly generation/tool handoff modeled after `Target::generate_assembly_with_opts_and_debug`, `assemble_with_extra`, and linker dispatch, but not the full `IrModule` pipeline yet

## Blockers

- none recorded

## Resume Notes

- do not activate the umbrella roadmap file directly
- keep the first slice limited to backend entry, adapter boundary, and trivial executable bring-up
- preserve the rule that Stage 3 handling stays inside the adapter layer, not target-specific code
- the backend insertion point should sit after `lir::lower(...)` and before `lir::print_llvm(...)`, so Step 2 can introduce a new subsystem boundary without perturbing HIR lowering yet
- avoid adapting directly to the ref backend `IrModule`; the first shim should consume our existing LIR container types and only narrow further when the backend entry API is in place
