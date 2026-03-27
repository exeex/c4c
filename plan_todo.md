# Backend LIR Adapter Todo

Status: Active
Source Idea: ideas/open/01_backend_lir_adapter_plan.md
Source Plan: plan.md

## Current Active Item

- Step 4: wire external assembler/linker fallback

## Next Intended Slice

- define the external tool handoff contract for backend-emitted LLVM text / assembly artifacts
- map the first supported target flow onto deterministic assembler/linker invocation and diagnostics
- extend the backend tests from adapter/runtime smoke coverage to explicit toolchain-failure attribution

## Completed Items

- activated `ideas/open/01_backend_lir_adapter_plan.md` into `plan.md`
- initialized execution-state tracking for the active plan
- Step 1 complete: current compiler handoff is `src/apps/c4cll.cpp -> c4c::codegen::llvm_backend::emit_module_native()` in `src/codegen/llvm/llvm_codegen.cpp`, which immediately calls `c4c::codegen::lir::lower(const c4c::hir::Module&)` and then `lir::print_llvm(...)`
- Step 1 complete: the current LIR-side adapter seam is centered on `src/codegen/lir/hir_to_lir.hpp`, `src/codegen/lir/ir.hpp`, and `src/codegen/lir/stmt_emitter.hpp`; the smallest backend-facing payload for return-only bring-up is `LirModule -> LirFunction -> LirBlock -> {LirInst, LirTerminator}`
- Step 1 complete: the minimal ref backend entry shape to mirror first is `ref/claudes-c-compiler/src/backend/mod.rs` `Target` + top-level dispatch, with future assembly generation/tool handoff modeled after `Target::generate_assembly_with_opts_and_debug`, `assemble_with_extra`, and linker dispatch, but not the full `IrModule` pipeline yet
- Step 2 complete: added `src/backend/target.{hpp,cpp}` and `src/backend/backend.{hpp,cpp}` so target-triple parsing and backend factory dispatch now live under a dedicated `src/backend/` boundary
- Step 2 complete: `src/codegen/llvm/llvm_codegen.cpp` now keeps `legacy` as the direct `lir::print_llvm(...)` path while routing `--codegen lir` and `--codegen compare` through `backend::emit_module(...)` with explicit target parsing
- Step 2 complete: added backend-entry validation in `tests/c/internal/InternalTests.cmake` for one supported target path and one unsupported-target diagnostic path; full `ctest` remained monotonic with the same 7 pre-existing failures before and after while total registered tests increased from 495 to 497
- Step 3 complete: added `src/backend/lir_adapter.{hpp,cpp}` as the first explicit backend-side contract, adapting the narrow return-only subset `LirModule -> BackendModule -> BackendFunction -> BackendBlock -> {BackendBinaryInst, BackendReturn}` instead of routing everything directly through `lir::print_llvm(...)`
- Step 3 complete: chose the first-pass Stage 3 handling policy to keep translation in the adapter layer only; `src/backend/backend.cpp` now renders the adapted return-only subset through the backend boundary and falls back to the legacy printer for out-of-scope LIR so the current tree keeps working while coverage grows
- Step 3 complete: added adapter-driven coverage in `tests/backend/backend_lir_adapter_tests.cpp` plus forced-`--codegen lir` runtime smoke cases `tests/c/internal/backend_case/return_zero.c` and `tests/c/internal/backend_case/return_add.c`
- Validation: targeted backend tests now pass for adapter unit coverage, supported/unsupported target entry, and runtime exit codes `0` and `5`
- Validation: `test_before.log` recorded 7 pre-existing failures out of 497 tests; `test_after.log` recorded the same 7 long-standing failures plus 3 unrelated C++ runtime failures out of 500 tests, and those 3 additional failures reproduce on a separate clean `HEAD` worktree (`template_arg_deduction`, `template_mixed_params`, `template_type_subst`)

## Blockers

- none recorded

## Resume Notes

- do not activate the umbrella roadmap file directly
- keep the first slice limited to backend entry, adapter boundary, and trivial executable bring-up
- preserve the rule that Stage 3 handling stays inside the adapter layer, not target-specific code
- the backend insertion point should sit after `lir::lower(...)` and before `lir::print_llvm(...)`, so Step 2 can introduce a new subsystem boundary without perturbing HIR lowering yet
- avoid adapting directly to the ref backend `IrModule`; the first shim should consume our existing LIR container types and only narrow further when the backend entry API is in place
- the new backend boundary currently returns the same LLVM text as the legacy LIR printer; Step 3 should replace that internal handoff with an adapter contract instead of growing target-specific logic in `llvm_codegen.cpp`
